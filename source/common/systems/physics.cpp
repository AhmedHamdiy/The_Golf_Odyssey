#pragma once

#include "physics.hpp"

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

    btRigidBody* PhysicsSystem::createRigidBody(float mass, const btTransform& startTransform, btCollisionShape* shape)
	{
		bool isDynamic = (mass != 0.f);
		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			shape->calculateLocalInertia(mass, localInertia);

		btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, localInertia);
		btRigidBody* body = new btRigidBody(info);
		body->setUserIndex(-1);
		dynamicsWorld->addRigidBody(body);
		return body;
	}

    PhysicsSystem::PhysicsSystem() {

        createDynamicsWorld();

        const btVector3 halfExtents();
        btBoxShape* groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));
        collisionShapes.push_back(groundShape);
    
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0, 0, 0));
        createRigidBody(btScalar(0.0f), groundTransform, groundShape);

        //TODO: Add all rigid bodies
    }

	void PhysicsSystem::stepSimulation(float deltaTime)
	{
		if (dynamicsWorld) dynamicsWorld->stepSimulation(deltaTime);
	}

    PhysicsSystem::~PhysicsSystem() {

            if (dynamicsWorld)
            {
                int i;
                for (i = dynamicsWorld->getNumConstraints() - 1; i >= 0; i--)
                {
                    dynamicsWorld->removeConstraint(dynamicsWorld->getConstraint(i));
                }
                for (i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
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
                delete collisionShapes[j];
            }
            collisionShapes.clear();
    
            delete dynamicsWorld;
            delete collisionConfiguration;
            delete dispatcher;
            delete solver;
            delete broadphase;
    }
}