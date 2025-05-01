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

    void PhysicsSystem::addTriangularMesh(std::vector<Vertex> &vertices, std::vector<unsigned int> &indices) {
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
        collisionShapes.push_back(shape);
        createRigidBody(btScalar(0.0f), glm::vec3(0, 0, 0), shape);
	}

    void PhysicsSystem::addConvexHullMesh(std::vector<Vertex> &vertices, float mass) {
        btConvexHullShape *shape = new btConvexHullShape(
            (btScalar*) &vertices[0].position,
            vertices.size(),
            sizeof(Vertex)
        );

        collisionShapes.push_back(shape);
        createRigidBody(btScalar(mass), glm::vec3(0, 0, 0), shape);
	}

    PhysicsSystem::PhysicsSystem() {
        createDynamicsWorld();
    }

    void PhysicsSystem::addComponents(World* world) {
        for(auto entity : world->getEntities()){
            MeshRendererComponent* meshRenderer = entity->getComponent<MeshRendererComponent>();
            if(meshRenderer) addTriangularMesh(meshRenderer->mesh->vertices, meshRenderer->mesh->elements);
        }
    }

    void PhysicsSystem::update(float deltaTime)
	{
		if (dynamicsWorld) {
            dynamicsWorld->stepSimulation(deltaTime);
            for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
            {
                btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
                btRigidBody* body = btRigidBody::upcast(obj);
                btTransform transform;
                if (body && body->getMotionState()) {
                    body->getMotionState()->getWorldTransform(transform);
                }
                else {
                    transform = obj->getWorldTransform();
                }
            }
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

            for (int j = 0; j < collisionShapes.size(); j++)
            {
                btCollisionShape* shape = collisionShapes[j];
                collisionShapes[j] = nullptr;
                delete shape;
            }
            
            delete dynamicsWorld;
            delete collisionConfiguration;
            delete dispatcher;
            delete solver;
            delete broadphase;
            collisionShapes.clear();
    }
}