#pragma once

#include "../ecs/world.hpp"
#include "../components/physics.hpp"
#include "../components/camera.hpp"

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

#define BT_LINE_BATCH_SIZE 512

namespace our {
    class PhysicsSystem {
        std::unordered_map<Entity *, btRigidBody *> collisionObjects;
        btDiscreteDynamicsWorld *dynamicsWorld;
        btDefaultCollisionConfiguration *collisionConfiguration;
        btCollisionDispatcher *dispatcher;
        btDbvtBroadphase *broadphase;
        btSequentialImpulseConstraintSolver *solver;

        void createDynamicsWorld();
        btRigidBody *createRigidBody(float mass, const glm::vec3 origin, btCollisionShape *shape,
                                     bool isKinematic = false);
        void addTriangularMesh(Entity *entity, const std::vector<Vertex> &vertices,
                               const std::vector<unsigned int> &indices);
        void addConvexHullMesh(Entity *entity, const std::vector<Vertex> &vertices);

      public:
        PhysicsSystem();
        void addComponents(World *world, glm::ivec2 windowSize);
        void physicsDebugDraw();
        std::unordered_map<Entity *, btRigidBody *> &getRigidBodies();
        void update(World *world, float deltaTime);
        ~PhysicsSystem();
    };
}