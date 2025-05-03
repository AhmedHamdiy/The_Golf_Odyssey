#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <systems/physics.hpp>
#include <asset-loader.hpp>
#include <mesh/mesh-utils.hpp>

#include "../common/components/camera.hpp"
#include "../common/components/movement.hpp"
#include "../common/ecs/entity.hpp"

const float VELOCITY_FACTOR = 0.3f;
const float MOUSE_TO_BALL_THRESHOLD = 30.0f;// 3ayza a3dl 3leha based on the dimensions of the ball??
const float BALL_RADIUS = 0.5f;
const float DECAY_RATE = 0.98f;
const float MIN_VELOCITY = 0.1f;
const float MAX_VELOCITY = 10.0f;

const glm::vec3 CAMERA_OFFSET(0.0f, 5.0f, 10.0f);


// This state shows how to use the ECS framework and deserialization.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::PhysicsSystem physicsSystem;

    bool ballDragging = false;
    glm::vec2 dragStart;

    void getNecessaryComponents(our::CameraComponent* &camera, our::Entity* &golfBall, our::Entity* &arrow){
        for (auto entity : world.getEntities()) {
            if (entity->name == "ball") golfBall = entity;
            else if (entity->name == "arrow") arrow = entity;
            else if(!camera) camera = entity->getComponent<our::CameraComponent>();  
        }
    }
    
    void updateCameraPosition(){
        our::Entity* golfBall = nullptr;
        our::CameraComponent* camera = nullptr;
        our::Entity* arrow = nullptr;
        getNecessaryComponents(camera,golfBall,arrow);

        if (golfBall && camera) {
            if(glm::length(golfBall->getComponent<our::MovementComponent>()->linearVelocity) < MIN_VELOCITY) return;
            glm::vec3 ballPos = golfBall->localTransform.position;
            glm::vec3 desiredCamPos = ballPos + CAMERA_OFFSET;
            camera->getOwner()->localTransform.position = desiredCamPos;
            camera->getOwner()->localTransform.rotation = glm::eulerAngles(glm::quatLookAt(glm::normalize(ballPos - desiredCamPos), glm::vec3(0.0f, 1.0f, 0.0f)));
        }
    }

    void updateBallVelocity(double deltaTime){
        our::Entity* golfBall = nullptr;
        our::CameraComponent* camera = nullptr;
        our::Entity* arrow = nullptr;
        getNecessaryComponents(camera,golfBall,arrow);

        auto golfMovementComponent = golfBall->getComponent<our::MovementComponent>();
        if(golfMovementComponent){
            float ballVelocity = glm::length(golfMovementComponent->linearVelocity);
            if(ballVelocity == 0.0f) return;

            golfMovementComponent->linearVelocity *= std::exp(-DECAY_RATE * deltaTime);
            golfMovementComponent->angularVelocity *= std::exp(-DECAY_RATE * deltaTime);
            if(ballVelocity < MIN_VELOCITY){
                golfMovementComponent->linearVelocity = glm::vec3(0.0f);
                golfMovementComponent->angularVelocity = glm::vec3(0.0f);
            }
        }
    }

    glm::vec3 getColorFromPower(float power, float maxPower = 300.0f) {
        float t = glm::clamp(power / maxPower, 0.0f, 1.0f);
        return glm::vec3(glm::min(2.0f * t, 1.0f), glm::min(2.0f * (1.0f - t), 1.0f), 0.0f);
    }

    void updateArrow(){
        our::Entity* golfBall = nullptr;
        our::CameraComponent* camera = nullptr;
        our::Entity* arrow = nullptr;
        getNecessaryComponents(camera, golfBall, arrow);
        if (!golfBall || !arrow || !camera) return;
    
        glm::vec2 mousePos = getApp()->getMouse().getMousePosition();
        glm::vec2 dragVec = dragStart -mousePos;
        float dragPower = glm::length(dragVec);
        // std::cout<<"power: "<<dragPower<<"\n";
        glm::mat4 viewMatrix = camera->getViewMatrix();
        glm::vec3 camRight = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
        glm::vec3 camForward = glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
    
        camForward.y = 0;
        camForward = glm::normalize(camForward);
        camRight = glm::normalize(camRight);
    
        glm::vec3 worldDir = -dragVec.x * camRight - dragVec.y * camForward;
    
        if(glm::length(worldDir) == 0) return;
        worldDir = glm::normalize(worldDir);
    
        float angle = atan2(worldDir.x, worldDir.z);
    
        arrow->localTransform.rotation = glm::vec3(-glm::half_pi<float>(), angle, -glm::pi<float>());
        arrow->localTransform.position = golfBall->localTransform.position + worldDir * BALL_RADIUS;
        arrow->localTransform.scale = glm::vec3(0.5f, dragPower * 0.01f, 0.5f);

        glm::vec3 color = getColorFromPower(dragPower);
        our::TintedMaterial* material = dynamic_cast<our::TintedMaterial*>(arrow->getComponent<our::MeshRendererComponent>()->material);
        if(material) material->tint = glm::vec4(color,1.0f);
    }
    
    

    void onInitialize() override {
        // First of all, we get the scene configuration from the app config
        auto& config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if(config.contains("assets")){
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if(config.contains("world")){
            world.deserialize(config["world"]);
        }
        // we add the physics system to the world
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        physicsSystem.addComponents(&world, size);
        renderer.initialize(size, config["renderer"]);
    }

    void onDraw(double deltaTime) override {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime, physicsSystem.getRigidBodies());
        if(!ballDragging)
        cameraController.update(&world, (float)deltaTime);
        // updateCameraPosition();
        if(ballDragging) updateArrow();
        updateBallVelocity(deltaTime);
        physicsSystem.update(&world, (float)deltaTime);
        
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);
        physicsSystem.physicsDebugDraw();

        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_ESCAPE)){
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }
    }

    void onDestroy() override {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }

    virtual void onMouseButtonEvent(int button, int action, int mods)override{
        if(button == GLFW_MOUSE_BUTTON_LEFT){
            our::Entity* golfBall = nullptr;
            our::CameraComponent* camera = nullptr;
            our::MovementComponent *golfMovementComponent = nullptr;
            our::Entity *arrow = nullptr;
            getNecessaryComponents(camera,golfBall,arrow);
            if(golfBall)
                golfMovementComponent = golfBall->getComponent<our::MovementComponent>();
            if(glm::length(golfMovementComponent->linearVelocity) > MIN_VELOCITY)
                return;
            if(action == GLFW_PRESS){
                glm::vec2 windowSize = renderer.windowSize;
                glm::vec3 ballWorldPos = golfBall->localTransform.position;
                glm::vec4 clipSpace = (camera->getProjectionMatrix(windowSize)) * (camera->getViewMatrix()) * glm::vec4(ballWorldPos, 1.0f);
                clipSpace /= clipSpace.w;
                float x = (clipSpace.x * 0.5f + 0.5f) * windowSize.x;
                float y = (1.0f - (clipSpace.y * 0.5f + 0.5f)) * windowSize.y;

                glm::vec2 ballPos = glm::vec2(x, y);
                glm::vec2 mousePos = getApp()->getMouse().getMousePosition();

                float distance = glm::distance(ballPos,mousePos);
                if(distance < MOUSE_TO_BALL_THRESHOLD){
                    dragStart = mousePos;
                    ballDragging = true;
                }
            }else if(action == GLFW_RELEASE && ballDragging){
                arrow->localTransform.scale = glm::vec3(0,0,0);

                ballDragging = false;
                glm::vec2 dragEnd = getApp()->getMouse().getMousePosition();
                glm::vec2 dragVec = dragEnd - dragStart;

                glm::mat4 viewMatrix = camera->getViewMatrix();
                glm::vec3 cameraRight = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
                glm::vec3 cameraUp = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
                glm::vec3 cameraForward = glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);

                cameraForward.y = 0;
                cameraForward = glm::normalize(cameraForward);
                glm::vec3 velocity = (-dragVec.x * cameraRight) + (-dragVec.y * cameraForward);
                velocity *= VELOCITY_FACTOR;

                if(golfMovementComponent){
                    golfMovementComponent->linearVelocity = velocity;
                    glm::vec3 rotationAxis = glm::normalize(glm::cross(velocity, glm::vec3(0, 1, 0)));
                    float angularSpeed = glm::length(velocity) / BALL_RADIUS;
                    golfMovementComponent->angularVelocity = angularSpeed * rotationAxis;
                }
            }
        }
    }

};