#include "physics.hpp"
#include "../components/mesh-renderer.hpp"

namespace our {

    struct Line {
        glm::vec3 from, to;
        glm::vec3 color;
    };
    class DebugDrawer : public btIDebugDraw {
        int debugMode;

        std::vector<Line> lines;
        ShaderProgram *shader;
        GLuint VAO, VBO;
        CameraComponent *camera = nullptr;
        glm::ivec2 windowSize;
        DefaultColors colors;

      public:
        DebugDrawer(CameraComponent *camera, glm::ivec2 windowSize)
            : debugMode(btIDebugDraw::DBG_DrawWireframe), camera(camera), windowSize(windowSize) {
            shader = new ShaderProgram();
            shader->attach("assets/shaders/line.vert", GL_VERTEX_SHADER);
            shader->attach("assets/shaders/line.frag", GL_FRAGMENT_SHADER);
            shader->link();
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
        }

        virtual ~DebugDrawer() {}
        virtual DefaultColors getDefaultColors() const { return colors; }
        virtual void setDefaultColors(const DefaultColors &colors) { this->colors = colors; }

        virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
            glm::vec3 fromVec(from.getX(), from.getY(), from.getZ());
            glm::vec3 toVec(to.getX(), to.getY(), to.getZ());
            glm::vec3 colorVec(color.getX(), color.getY(), color.getZ());

            lines.push_back({fromVec, toVec, colorVec});
        }

        virtual void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB,
                                      btScalar distance, int lifeTime, const btVector3 &color) {}
        virtual void reportErrorWarning(const char *warningString) {}
        virtual void draw3dText(const btVector3 &location, const char *textString) {}

        virtual void setDebugMode(int debugMode) { debugMode = debugMode; }
        virtual int getDebugMode() const { return debugMode; }

        virtual void flushLines() {
            if (!lines.size())
                return;

            std::vector<float> vertices;
            for (auto &line : lines) {
                vertices.insert(vertices.end(), {line.from.x, line.from.y, line.from.z,
                                                 line.color.r, line.color.g, line.color.b});
                vertices.insert(vertices.end(), {line.to.x, line.to.y, line.to.z, line.color.r,
                                                 line.color.g, line.color.b});
            }

            glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(),
                         GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                                  (void *)(3 * sizeof(float)));
            shader->use();
            shader->set("VP", VP);
            glDrawArrays(GL_LINES, 0, 2 * lines.size());

            glBindVertexArray(0);
            lines.clear();
        }
    };

    void PhysicsSystem::createDynamicsWorld() {

        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);
        broadphase = new btDbvtBroadphase();
        solver = new btSequentialImpulseConstraintSolver;
        dynamicsWorld =
            new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0, -9.8, 0));
    }

    btRigidBody *PhysicsSystem::createRigidBody(float mass, const glm::vec3 origin,
                                                btCollisionShape *shape, bool isKinematic) {
        bool isDynamic = (mass != 0.f);
        btVector3 localInertia(0, 0, 0);
        if (isDynamic)
            shape->calculateLocalInertia(mass, localInertia);

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(origin.x, origin.y, origin.z));
        btDefaultMotionState *motionState = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, localInertia);
        btRigidBody *body = new btRigidBody(info);
        body->setUserIndex(-1);
        if (isKinematic)
            body->setCollisionFlags(body->getCollisionFlags() |
                                    btCollisionObject::CF_KINEMATIC_OBJECT);
        if (isDynamic) {
            body->setFriction(0.6f);
            body->setRestitution(0.1f);
            body->setRollingFriction(0.1f);
            body->setSpinningFriction(0.05f);
            body->setDamping(0.05f, 0.8f);
        } else {
            body->setFriction(0.8f);
            body->setRestitution(0.8f);
        }
        dynamicsWorld->addRigidBody(body);
        return body;
    }

    void PhysicsSystem::addTriangularMesh(Entity *entity, const std::vector<Vertex> &vertices,
                                          const std::vector<unsigned int> &indices) {
        btTriangleMesh *triangleMesh = new btTriangleMesh();
        for (int i = 0; i < indices.size(); i += 3) {
            glm::vec3 p1 = vertices[indices[i]].position;
            glm::vec3 p2 = vertices[indices[i + 1]].position;
            glm::vec3 p3 = vertices[indices[i + 2]].position;

            triangleMesh->addTriangle(btVector3(p1.x, p1.y, p1.z), btVector3(p2.x, p2.y, p2.z),
                                      btVector3(p3.x, p3.y, p3.z));
        }
        btCollisionShape *shape = new btBvhTriangleMeshShape(triangleMesh, true, true);
        glm::vec3 scale = entity->localTransform.scale;
        shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
        btScalar mass = btScalar(entity->getComponent<PhysicsComponent>()->mass);
        glm::vec3 origin = entity->localTransform.position;
        btRigidBody *body = createRigidBody(mass, origin, shape);
        collisionObjects[entity] = body;
    }

    void PhysicsSystem::addConvexHullMesh(Entity *entity, const std::vector<Vertex> &vertices) {
        btConvexHullShape *shape = new btConvexHullShape((btScalar *)&vertices[0].position,
                                                         vertices.size(), sizeof(Vertex));
        glm::vec3 scale = entity->localTransform.scale;
        shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
        btScalar mass = btScalar(entity->getComponent<PhysicsComponent>()->mass);
        glm::vec3 origin = entity->localTransform.position;
        btRigidBody *body = createRigidBody(mass, origin, shape);
        collisionObjects[entity] = body;
    }

    void PhysicsSystem::physicsDebugDraw() {
        if (!dynamicsWorld)
            return;
        dynamicsWorld->debugDrawWorld();
    }

    PhysicsSystem::PhysicsSystem() { createDynamicsWorld(); }

    void PhysicsSystem::addComponents(World *world, glm::ivec2 windowSize) {
        CameraComponent *camera = nullptr;
        for (auto entity : world->getEntities()) {
            if (!camera)
                camera = entity->getComponent<CameraComponent>();
            if (PhysicsComponent *physics = entity->getComponent<PhysicsComponent>(); physics) {
                if (MeshRendererComponent *meshRenderer =
                        entity->getComponent<MeshRendererComponent>();
                    meshRenderer) {
                    if (entity->name == "ball")
                        addConvexHullMesh(entity, meshRenderer->mesh->vertices);
                    else
                        addTriangularMesh(entity, meshRenderer->mesh->vertices,
                                          meshRenderer->mesh->elements);
                }
            }
        }
        DebugDrawer *debugDrawer = new DebugDrawer(camera, windowSize);
        dynamicsWorld->setDebugDrawer(debugDrawer);
    }

    std::unordered_map<Entity *, btRigidBody *> &PhysicsSystem::getRigidBodies() {
        return collisionObjects;
    }

    void PhysicsSystem::update(World *world, float deltaTime) {
        if (!dynamicsWorld)
            return;
        dynamicsWorld->stepSimulation(deltaTime, 10);

        for (auto &obj : collisionObjects) {
            btRigidBody *body = obj.second;
            btTransform transform;
            Entity *entity = obj.first;

            if (!body || !entity)
                continue;

            if (body->getMotionState())
                body->getMotionState()->getWorldTransform(transform);
            else
                transform = body->getWorldTransform();

            btVector3 position = transform.getOrigin();
            btQuaternion rotation = transform.getRotation();

            entity->localTransform.position =
                glm::vec3(position.getX(), position.getY(), position.getZ());
            entity->localTransform.rotation = glm::eulerAngles(
                glm::quat(rotation.getW(), rotation.getX(), rotation.getY(), rotation.getZ()));
        }
    }

    PhysicsSystem::~PhysicsSystem() {

        if (dynamicsWorld) {
            for (int i = dynamicsWorld->getNumConstraints() - 1; i >= 0; i--) {
                dynamicsWorld->removeConstraint(dynamicsWorld->getConstraint(i));
            }
            for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
                btCollisionObject *obj = dynamicsWorld->getCollisionObjectArray()[i];
                btRigidBody *body = btRigidBody::upcast(obj);
                if (body && body->getMotionState()) {
                    delete body->getMotionState();
                }
                dynamicsWorld->removeCollisionObject(obj);
                delete obj;
            }
        }

        delete dynamicsWorld;
        delete collisionConfiguration;
        delete dispatcher;
        delete solver;
        delete broadphase;
    }
}