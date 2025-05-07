#pragma once

#include <chrono>
#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <systems/physics.hpp>
#include <asset-loader.hpp>
#include <mesh/mesh-utils.hpp>
#include <shader/shader.hpp>
#include <material/material.hpp>
#include <mesh/mesh.hpp>
#include <texture/texture2d.hpp>
#include <texture/texture-utils.hpp>

#include "../common/components/camera.hpp"
#include "../common/components/movement.hpp"
#include "../common/ecs/entity.hpp"

const float VELOCITY_FACTOR = 0.2f;
const float MOUSE_TO_BALL_THRESHOLD = 30.0f;
const float BALL_RADIUS = 0.5f;
const float DECAY_RATE = 0.98f;
const float MIN_VELOCITY = 0.5f;
const float MAX_VELOCITY = 300.0f;
const float MAX_POWER = 300.0f;
const int MAX_STROKES = 15;
const int MAX_TIME = 300;
const int FELL_THRESHOLD = -17;
const glm::vec3 CAMERA_OFFSET(0.0f, 5.0f, 10.0f);

// This state shows how to use the ECS framework and deserialization.
class Playstate : public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::PhysicsSystem physicsSystem;

    // Button rendering resources
    our::TexturedMaterial *buttonMaterial;
    our::Mesh *buttonRectangle;
    Button menuButton;

    bool ballDragging = false;
    bool won = false;
    int strokesNum = 0;
    glm::vec2 dragStart;
    bool isMoving = false;
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point currentTime;
    int counter = 0;

    void getNecessaryComponents(our::CameraComponent *&camera, our::Entity *&golfBall,
                                our::Entity *&arrow) {
        for (auto entity : world.getEntities()) {
            if (entity->name == "ball")
                golfBall = entity;
            else if (entity->name == "arrow")
                arrow = entity;
            else if (!camera)
                camera = entity->getComponent<our::CameraComponent>();
        }
    }

    void updateState(int time, bool fell, our::Entity *golfBall) {
        if (time >= MAX_TIME || (strokesNum >= MAX_STROKES && !won && !isMoving)) {
            return getApp()->changeState("lose");
        }
        if (won) {
            return getApp()->changeState("win");
        }
        if (fell) {
            // Check if ball's x,z coordinates are within the circle
            // Circle center: (-237, -106), radius: 5
            glm::vec3 ballPos = golfBall->localTransform.position;
            float h = -237.0f;
            float k = -106.0f;
            float r = 10.0f;
            float distanceSquared =
                (ballPos.x - h) * (ballPos.x - h) + (ballPos.z - k) * (ballPos.z - k);
            if (distanceSquared <= r * r) {
                won = true;
                return getApp()->changeState("win");
            } else {
                return getApp()->changeState("lose");
            }
        }
    }

    void updateCameraPosition() {
        our::Entity *golfBall = nullptr;
        our::CameraComponent *camera = nullptr;
        our::Entity *arrow = nullptr;
        getNecessaryComponents(camera, golfBall, arrow);

        if (golfBall && camera) {
            if (glm::length(golfBall->getComponent<our::MovementComponent>()->linearVelocity) <
                MIN_VELOCITY)
                return;
            glm::vec3 ballPos = golfBall->localTransform.position;
            glm::vec3 desiredCamPos = ballPos + CAMERA_OFFSET;
            camera->getOwner()->localTransform.position = desiredCamPos;
            camera->getOwner()->localTransform.rotation = glm::eulerAngles(glm::quatLookAt(
                glm::normalize(ballPos - desiredCamPos), glm::vec3(0.0f, 1.0f, 0.0f)));
        }
    }

    void updateBallVelocity(double deltaTime) {
        our::Entity *golfBall = nullptr;
        our::CameraComponent *camera = nullptr;
        our::Entity *arrow = nullptr;
        getNecessaryComponents(camera, golfBall, arrow);
        auto golfMovementComponent = golfBall->getComponent<our::MovementComponent>();
        if (golfMovementComponent) {
            std::cout << "max velocity: " << glm::length(golfMovementComponent->linearVelocity)
                      << "\n";
            float ballVelocity = glm::length(golfMovementComponent->linearVelocity);
            if (ballVelocity == 0.0f)
                return;

            golfMovementComponent->linearVelocity *= std::exp(-DECAY_RATE * deltaTime);
            golfMovementComponent->angularVelocity *= std::exp(-DECAY_RATE * deltaTime);
            if (ballVelocity < MIN_VELOCITY) {
                golfMovementComponent->linearVelocity = glm::vec3(0.0f);
                golfMovementComponent->angularVelocity = glm::vec3(0.0f);
            } else if (ballVelocity > MAX_VELOCITY) {
                golfMovementComponent->linearVelocity =
                    MAX_VELOCITY * glm::normalize(golfMovementComponent->linearVelocity);
                golfMovementComponent->angularVelocity =
                    MAX_VELOCITY * glm::normalize(golfMovementComponent->linearVelocity);
            }
        }
    }

    glm::vec3 getColorFromPower(float power, float maxPower = 300.0f) {
        float t = glm::clamp(power / maxPower, 0.0f, 1.0f);
        return glm::vec3(glm::min(2.0f * t, 1.0f), glm::min(2.0f * (1.0f - t), 1.0f), 0.0f);
    }

    void updateArrow() {
        our::Entity *golfBall = nullptr;
        our::CameraComponent *camera = nullptr;
        our::Entity *arrow = nullptr;
        getNecessaryComponents(camera, golfBall, arrow);
        if (!golfBall || !arrow || !camera)
            return;

        glm::vec2 mousePos = getApp()->getMouse().getMousePosition();
        glm::vec2 dragVec = dragStart - mousePos;
        float dragPower = glm::min(glm::length(dragVec), MAX_POWER);
        std::cout << "power: " << dragPower << "\n";
        glm::mat4 viewMatrix = camera->getViewMatrix();
        glm::vec3 camRight = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
        glm::vec3 camForward = glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);

        camForward.y = 0;
        camForward = glm::normalize(camForward);
        camRight = glm::normalize(camRight);

        glm::vec3 worldDir = -dragVec.x * camRight - dragVec.y * camForward;

        if (glm::length(worldDir) == 0)
            return;
        worldDir = glm::normalize(worldDir);

        float angle = atan2(worldDir.x, worldDir.z);

        arrow->localTransform.rotation =
            glm::vec3(-glm::half_pi<float>(), angle, -glm::pi<float>());
        arrow->localTransform.position = golfBall->localTransform.position + worldDir * BALL_RADIUS;
        arrow->localTransform.scale = glm::vec3(0.5f, dragPower * 0.01f, 0.5f);

        glm::vec3 color = getColorFromPower(dragPower);
        our::TintedMaterial *material = dynamic_cast<our::TintedMaterial *>(
            arrow->getComponent<our::MeshRendererComponent>()->material);
        if (material)
            material->tint = glm::vec4(color, 1.0f);
    }

    void autoRelease() {
        glm::vec2 dragEnd = getApp()->getMouse().getMousePosition();
        glm::vec2 dragVec = dragEnd - dragStart;

        if (glm::length(dragVec) == 0.0f)
            return;
        if (glm::length(dragVec) >= MAX_VELOCITY)
            onMouseButtonEvent(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
    }

    void onInitialize() override {
        ballDragging = false;
        won = false;
        strokesNum = 0;
        isMoving = false;
        counter = 0;

        // First of all, we get the scene configuration from the app config
        auto &config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if (config.contains("assets")) {
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if (config.contains("world")) {
            world.deserialize(config["world"]);
        }
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        physicsSystem.addComponents(&world, size);
        renderer.initialize(size, config["renderer"]);
        startTime = std::chrono::steady_clock::now();

        renderer.set_fog_power(0.7f);
        // Initialize button material
        buttonMaterial = new our::TexturedMaterial();
        buttonMaterial->shader = new our::ShaderProgram();
        buttonMaterial->shader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
        buttonMaterial->shader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
        buttonMaterial->shader->link();
        buttonMaterial->texture = our::texture_utils::loadImage("assets/textures/home.png");
        buttonMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Full color texture
        buttonMaterial->pipelineState.blending.enabled = true;
        buttonMaterial->pipelineState.blending.equation = GL_FUNC_ADD;
        buttonMaterial->pipelineState.blending.sourceFactor = GL_SRC_ALPHA;
        buttonMaterial->pipelineState.blending.destinationFactor = GL_ONE_MINUS_SRC_ALPHA;

        // Initialize button rectangle
        buttonRectangle = new our::Mesh(
            {
                {{0.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                {{1.0f, 0.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                {{1.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                {{0.0f, 1.0f, 0.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            },
            {
                0,
                1,
                2,
                2,
                3,
                0,
            });

        renderer.set_fog_power(1.0f);

        // Initialize menu button (top-left corner, 100x50 pixels)
        menuButton.position = {640.0f, 10.0f};
        menuButton.size = {50.0f, 50.0f};
        menuButton.action = [this]() { this->getApp()->changeState("menu"); };
    }

    void onDraw(double deltaTime) override {
        // Here, we just run a bunch of systems to control the world logic
        currentTime = std::chrono::steady_clock::now();
        int elapsed = std::chrono::duration<float>(currentTime - startTime).count();

        our::Entity *golfBall = nullptr;
        our::CameraComponent *camera = nullptr;
        our::MovementComponent *golfMovementComponent = nullptr;
        our::Entity *arrow = nullptr;
        getNecessaryComponents(camera, golfBall, arrow);

        std::cout << "Position: (" << golfBall->localTransform.position.x << ", "
                  << golfBall->localTransform.position.y << ", "
                  << golfBall->localTransform.position.z << ")\n";

        int remainingTime = MAX_TIME - elapsed;
        int minutes = remainingTime / 60;
        int seconds = remainingTime % 60;
        std::cout << "Time Remaining: " << minutes << ":" << (seconds < 10 ? "0" : "") << seconds
                  << "\n";
        bool fell = golfBall->localTransform.position.y < FELL_THRESHOLD ? true : false;
        btRigidBody *body = physicsSystem.getRigidBodies()[golfBall];
        updateState(elapsed, fell, golfBall);
        if (elapsed >= MAX_TIME || (strokesNum >= MAX_STROKES && !won && !isMoving) || won || fell)
            return;
        movementSystem.update(&world, (float)deltaTime, physicsSystem.getRigidBodies());
        if (!ballDragging)
            cameraController.update(&world, (float)deltaTime);
        // updateCameraPosition();
        if (ballDragging)
            updateArrow();
        autoRelease();
        // updateBallVelocity(deltaTime);
        physicsSystem.update(&world, (float)deltaTime);

        if (body->getLinearVelocity().length() == 0.0f && ++counter > 5) {
            isMoving = false;
            counter = 0;
        } else
            isMoving = true;
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Render button
        glm::ivec2 size = getApp()->getFrameBufferSize();
        glViewport(0, 0, size.x, size.y);
        glm::mat4 VP = glm::ortho(0.0f, (float)size.x, (float)size.y, 0.0f, 1.0f, -1.0f);
        buttonMaterial->setup();
        buttonMaterial->shader->set("transform", VP * menuButton.getLocalToWorld());
        buttonRectangle->draw();

        // Handle button interaction
        auto &mouse = getApp()->getMouse();
        glm::vec2 mousePosition = mouse.getMousePosition();
        if (mouse.justPressed(0) && menuButton.isInside(mousePosition)) {
            menuButton.action();
        }

        auto &keyboard = getApp()->getKeyboard();
        if (keyboard.justPressed(GLFW_KEY_ESCAPE)) {
            getApp()->changeState("menu");
        }
    }

    void onDestroy() override {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is
        // unlocked
        cameraController.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();

        // Clean up button resources
        delete buttonRectangle;
        delete buttonMaterial->texture;
        delete buttonMaterial->shader;
        delete buttonMaterial;
    }

    virtual void onMouseButtonEvent(int button, int action, int mods) override {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            our::Entity *golfBall = nullptr;
            our::CameraComponent *camera = nullptr;
            our::MovementComponent *golfMovementComponent = nullptr;
            our::Entity *arrow = nullptr;
            getNecessaryComponents(camera, golfBall, arrow);
            btRigidBody *body = physicsSystem.getRigidBodies()[golfBall];

            if (golfBall)
                golfMovementComponent = golfBall->getComponent<our::MovementComponent>();
            if (glm::length(body->getLinearVelocity().length()) > MIN_VELOCITY)
                return;
            if (action == GLFW_PRESS) {
                glm::vec2 windowSize = renderer.windowSize;
                glm::vec3 ballWorldPos = golfBall->localTransform.position;
                glm::vec4 clipSpace = (camera->getProjectionMatrix(windowSize)) *
                                      (camera->getViewMatrix()) * glm::vec4(ballWorldPos, 1.0f);
                clipSpace /= clipSpace.w;
                float x = (clipSpace.x * 0.5f + 0.5f) * windowSize.x;
                float y = (1.0f - (clipSpace.y * 0.5f + 0.5f)) * windowSize.y;

                glm::vec2 ballPos = glm::vec2(x, y);
                glm::vec2 mousePos = getApp()->getMouse().getMousePosition();

                float distance = glm::distance(ballPos, mousePos);
                if (distance < MOUSE_TO_BALL_THRESHOLD) {
                    dragStart = ballPos;
                    ballDragging = true;
                    isMoving = true;
                }
            } else if (action == GLFW_RELEASE && ballDragging) {
                strokesNum++;
                arrow->localTransform.scale = glm::vec3(0, 0, 0);

                ballDragging = false;
                glm::vec2 dragEnd = getApp()->getMouse().getMousePosition();
                glm::vec2 dragVec = dragEnd - dragStart;

                if (glm::length(dragVec) == 0.0f)
                    return;
                if (glm::length(dragVec) >= MAX_VELOCITY)
                    dragVec = MAX_VELOCITY * glm::normalize(dragVec);

                glm::mat4 viewMatrix = camera->getViewMatrix();
                glm::vec3 cameraRight =
                    glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
                glm::vec3 cameraUp =
                    glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
                glm::vec3 cameraForward =
                    glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);

                cameraForward.y = 0;
                cameraForward = glm::normalize(cameraForward);
                glm::vec3 velocity = (-dragVec.x * cameraRight) + (-dragVec.y * cameraForward);
                velocity *= VELOCITY_FACTOR;
                std::cout << "velocity: " << glm::length(velocity) << "\n";

                if (golfMovementComponent) {
                    golfMovementComponent->linearVelocity = velocity;
                    glm::vec3 rotationAxis =
                        glm::normalize(glm::cross(velocity, glm::vec3(0, 1, 0)));
                    float angularSpeed = glm::length(velocity) / BALL_RADIUS;
                    golfMovementComponent->angularVelocity = angularSpeed * rotationAxis;
                }
            }
        }
    }
};