#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <asset-loader.hpp>

#include "../common/ecs/entity.hpp"

const float VELOCITY_FACTOR = 0.1f;

// This state shows how to use the ECS framework and deserialization.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;

    bool ballDragging = false;
    glm::vec2 dragStart;

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
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
    }

    void onDraw(double deltaTime) override {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

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

    virtual void onKeyEvent(int key, int scancode, int action, int mods) override{}      
    virtual void onCursorMoveEvent(double x, double y)override{}
    virtual void onCursorEnterEvent(int entered)override{}
    virtual void onMouseButtonEvent(int button, int action, int mods)override{
        if(button == GLFW_MOUSE_BUTTON_LEFT){
            if(action == GLFW_PRESS){
                dragStart = getApp()->getMouse().getMousePosition();
                ballDragging = true;
            }else if(action == GLFW_RELEASE && ballDragging){
                ballDragging = false;
                glm::vec2 dragEnd = getApp()->getMouse().getMousePosition();
                glm::vec2 dragVec = dragEnd - dragStart;
                glm::vec3 velocity(-dragVec.x,0,-dragVec.y);
                velocity *= VELOCITY_FACTOR;
                
                our::Entity* golfBall = nullptr;
                for (auto entity : world.getEntities()) {
                    if (entity->name == "ball"){
                        golfBall = entity;
                        break;
                    }
                }
                if(golfBall){
                    auto golfMovementComponent = golfBall->getComponent<our::MovementComponent>();
                    if(golfMovementComponent) golfMovementComponent->linearVelocity = velocity;
                }
            }
        }
    }
    virtual void onScrollEvent(double x_offset, double y_offset)override{}

};