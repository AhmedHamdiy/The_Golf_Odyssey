#pragma once

#include "../ecs/world.hpp"
#include "../components/physics.hpp"


#include <btBulletDynamicsCommon.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

namespace our
{
    class PhysicsSystem {
        std::unordered_map<btCollisionObject*, Entity*> collisionObjects;
        btDiscreteDynamicsWorld *dynamicsWorld;
        btDefaultCollisionConfiguration *collisionConfiguration;
		btCollisionDispatcher *dispatcher;
		btDbvtBroadphase *broadphase;
		btSequentialImpulseConstraintSolver* solver;

        void createDynamicsWorld();
        btRigidBody* createRigidBody(float mass, const glm::vec3 origin, btCollisionShape* shape);
        void addTriangularMesh(Entity *entity, const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices);
        void addConvexHullMesh(Entity *entity, const std::vector<Vertex> &vertices, float mass);
    public:
        PhysicsSystem();
        void PhysicsSystem::addComponents(World* world);
        void update(World* world, float deltaTime);
        ~PhysicsSystem();
    };
}