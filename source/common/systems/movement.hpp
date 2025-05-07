#pragma once

#include "../ecs/world.hpp"
#include "../components/movement.hpp"
#include "../components/physics.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <unordered_map>
#include <btBulletDynamicsCommon.h>
#include <iostream>

namespace our {

    // The movement system is responsible for moving every entity which contains a
    // MovementComponent. This system is added as a simple example for how use the ECS framework to
    // implement logic. For more information, see "common/components/movement.hpp"
    class MovementSystem {
      public:
        // This should be called every frame to update all entities containing a MovementComponent.
        void update(World *world, float deltaTime,
                    std::unordered_map<Entity *, btRigidBody *> &entityToRigidBody) {
            // For each entity in the world
            for (auto entity : world->getEntities()) {
                // Get the movement component if it exists
                MovementComponent *movement = entity->getComponent<MovementComponent>();
                if (!movement)
                    continue;

                PhysicsComponent *physics = entity->getComponent<PhysicsComponent>();
                if (!physics) {
                    // Change the position and rotation based on the linear & angular velocity and
                    // delta time.
                    entity->localTransform.position += deltaTime * movement->linearVelocity;
                    entity->localTransform.rotation += deltaTime * movement->angularVelocity;
                    continue;
                }

                btRigidBody *body = entityToRigidBody[entity];
                if (!body || body->isStaticObject())
                    continue;

                if (body->isKinematicObject()) {
                    entity->localTransform.position += deltaTime * movement->linearVelocity;
                    entity->localTransform.rotation += deltaTime * movement->angularVelocity;
                    body->activate(true);
                    body->setWorldTransform(
                        btTransform(btQuaternion(entity->localTransform.rotation.x,
                                                 entity->localTransform.rotation.y,
                                                 entity->localTransform.rotation.z),
                                    btVector3(entity->localTransform.position.x,
                                              entity->localTransform.position.y,
                                              entity->localTransform.position.z)));
                    continue;
                }
                if (movement->linearVelocity == glm::vec3(0.0f) &&
                    movement->angularVelocity == glm::vec3(0.0f))
                    continue;

                body->activate(true);

                body->setLinearVelocity(btVector3(movement->linearVelocity.x,
                                                  movement->linearVelocity.y,
                                                  movement->linearVelocity.z));

                body->setAngularVelocity(btVector3(movement->angularVelocity.x,
                                                   movement->angularVelocity.y,
                                                   movement->angularVelocity.z));

                movement->linearVelocity = glm::vec3(0.0f);
                movement->angularVelocity = glm::vec3(0.0f);
            }
        }
    };

}
