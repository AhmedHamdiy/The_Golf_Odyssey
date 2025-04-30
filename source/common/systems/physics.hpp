#pragma once

#include "../ecs/world.hpp"
#include "../components/physics.hpp"


#include <btBulletDynamicsCommon.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

namespace our
{
    class PhysicsSystem {
        btAlignedObjectArray<btCollisionShape*> collisionShapes;
        btDiscreteDynamicsWorld *dynamicsWorld;
        btDefaultCollisionConfiguration *collisionConfiguration;
		btCollisionDispatcher *dispatcher;
		btDbvtBroadphase *broadphase;
		btSequentialImpulseConstraintSolver* solver;

        void createDynamicsWorld();
        btRigidBody* createRigidBody(float mass, const btTransform& startTransform, btCollisionShape* shape);
    public:
        PhysicsSystem();
        void stepSimulation(float deltaTime);
        ~PhysicsSystem();
    };
}