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
        btRigidBody* createRigidBody(float mass, const glm::vec3 origin, btCollisionShape* shape);
        void addTriangularMesh(std::vector<Vertex> &vertices, std::vector<unsigned int> &indices);
        void addConvexHullMesh(std::vector<Vertex> &vertices, float mass);
        public:
        PhysicsSystem();
        void PhysicsSystem::addComponents(World* world);
        void update(float deltaTime);
        ~PhysicsSystem();
    };
}