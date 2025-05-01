#pragma once

#include "physics.hpp"
#include "../components/mesh-renderer.hpp"

namespace our
{
    void PhysicsSystem::createDynamicsWorld() {

		collisionConfiguration = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher(collisionConfiguration);
		broadphase = new btDbvtBroadphase();
		solver = new btSequentialImpulseConstraintSolver;
		dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
		dynamicsWorld->setGravity(btVector3(0, -9.8, 0));
    }

    btRigidBody* PhysicsSystem::createRigidBody(float mass, const glm::vec3 origin, btCollisionShape* shape) {
		bool isDynamic = (mass != 0.f);
		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			shape->calculateLocalInertia(mass, localInertia);

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(origin.x, origin.y, origin.z));
		btDefaultMotionState* motionState = new btDefaultMotionState(transform);
		btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, localInertia);
		btRigidBody* body = new btRigidBody(info);
		body->setUserIndex(-1);
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		dynamicsWorld->addRigidBody(body);
		return body;
	}

    void PhysicsSystem::addTriangularMesh(Entity *entity, const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices) {
        btTriangleMesh * triangleMesh = new btTriangleMesh();
        for (int i = 0; i < indices.size(); i += 3) {
            glm::vec3 p1 = vertices[indices[i]].position;
            glm::vec3 p2 = vertices[indices[i+1]].position;
            glm::vec3 p3 = vertices[indices[i+2]].position;
            
            triangleMesh->addTriangle(
                btVector3(p1.x, p1.y, p1.z),
                btVector3(p2.x, p2.y, p2.z),
                btVector3(p3.x, p3.y, p3.z)
            );
        }
        btCollisionShape* shape = new btBvhTriangleMeshShape(triangleMesh, true, true);
        btCollisionObject* body = createRigidBody(btScalar(0.0f), glm::vec3(0, 0, 0), shape);
        collisionObjects[body] = entity;
	}

    void PhysicsSystem::addConvexHullMesh(Entity *entity, const std::vector<Vertex> &vertices, float mass) {
        btConvexHullShape *shape = new btConvexHullShape(
            (btScalar*) &vertices[0].position,
            vertices.size(),
            sizeof(Vertex)
        );
        btCollisionObject* body = createRigidBody(btScalar(mass), glm::vec3(0, 0, 0), shape);
        collisionObjects[body] = entity;
	}

    PhysicsSystem::PhysicsSystem() {
        createDynamicsWorld();
    }

    void PhysicsSystem::addComponents(World* world) {
        for(auto entity : world->getEntities()) {
            if(PhysicsComponent *physics = entity->getComponent<PhysicsComponent>(); physics) {
                if (MeshRendererComponent *meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer)
                    addTriangularMesh(entity, meshRenderer->mesh->vertices, meshRenderer->mesh->elements);
            }
        }
    }

    void PhysicsSystem::update(World* world, float deltaTime)
	{
		if (dynamicsWorld) {
            for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
            {
                btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
                btRigidBody* body = btRigidBody::upcast(obj);
                btTransform transform;
                if (body && body->getMotionState()) {
                    body->getMotionState()->getWorldTransform(transform);
                    Entity* entity = collisionObjects[obj];
                    btVector3 position = transform.getOrigin();
                    btQuaternion rotation = transform.getRotation();
                    entity->localTransform.position = glm::vec3(position.getX(), position.getY(), position.getZ());
                    entity->localTransform.rotation = glm::vec3(rotation.getX(), rotation.getY(), rotation.getZ());
                }
                else {
                    transform = obj->getWorldTransform();
                    Entity* entity = collisionObjects[obj];
                    glm::vec3 position = entity->localTransform.position;
                    glm::vec3 rotation = entity->localTransform.rotation;
                    transform.setOrigin(btVector3(position.x, position.y, position.z));
                    transform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, 1.0f));
                    obj->setWorldTransform(transform);
                }
            }
            dynamicsWorld->stepSimulation(deltaTime);
        }
	}

    PhysicsSystem::~PhysicsSystem() {

            if (dynamicsWorld)
            {
                for (int i = dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
                {
                    dynamicsWorld->removeConstraint(dynamicsWorld->getConstraint(i));
                }
                for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
                {
                    btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
                    btRigidBody* body = btRigidBody::upcast(obj);
                    if (body && body->getMotionState())
                    {
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