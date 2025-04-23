#include <glm/gtx/euler_angles.hpp>
#include "forward-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"
#include "../deserialize-utils.hpp"

#include <iostream>

namespace our {

    void ForwardRenderer::setupCommand(RenderCommand &c, std::vector<LightComponent *> &lights,
                                       glm::mat4 VP, glm::vec3 cameraPosition) {
        c.material->setup();
        c.material->shader->set("M_transform", c.localToWorld);
        c.material->shader->set("M_invT_transform", glm::inverse(c.localToWorld), GL_TRUE);
        c.material->shader->set("VP_transform", VP);
        c.material->shader->set("camera_position", cameraPosition);
        c.material->shader->set("sky_light_color.top", skyLight.top_color);
        c.material->shader->set("sky_light_color.middle", skyLight.middle_color);
        c.material->shader->set("sky_light_color.bottom", skyLight.bottom_color);

        int index = 0;
        for (const auto &lightComponent : lights) {
            for (const auto &light : lightComponent->lights) {
                if (!light.enabled)
                    continue;
                std::string prefix = "lights[" + std::to_string(index) + "].";

                c.material->shader->set(prefix + "type", static_cast<int>(light.type));
                c.material->shader->set(prefix + "color", light.color);

                const glm::vec3 position = lightComponent->getOwner()->localTransform.position;
                const glm::vec3 rotation = lightComponent->getOwner()->localTransform.rotation;
                const glm::mat4 rotationMatrix = glm::yawPitchRoll(rotation.x, rotation.y, rotation.z);
                const glm::vec3 direction = glm::vec3(rotationMatrix * glm::vec4(0, 0, -1, 0));
                switch (light.type) {
                case LightType::DIRECTIONAL:
                    c.material->shader->set(prefix + "direction", glm::normalize(direction));
                    break;
                case LightType::POINT:
                    c.material->shader->set(prefix + "position", position);
                    c.material->shader->set(prefix + "attenuation.constant",
                                            light.attenuation.constant);
                    c.material->shader->set(prefix + "attenuation.linear",
                                            light.attenuation.linear);
                    c.material->shader->set(prefix + "attenuation.quadratic",
                                            light.attenuation.quadratic);
                    break;
                case LightType::SPOT:
                    c.material->shader->set(prefix + "position", position);
                    c.material->shader->set(prefix + "direction", glm::vec3(direction));
                    c.material->shader->set(prefix + "attenuation.constant",
                                            light.attenuation.constant);
                    c.material->shader->set(prefix + "attenuation.linear",
                                            light.attenuation.linear);
                    c.material->shader->set(prefix + "attenuation.quadratic",
                                            light.attenuation.quadratic);
                    c.material->shader->set(prefix + "spotAngle.inner", light.spotAngle.inner);
                    c.material->shader->set(prefix + "spotAngle.outer", light.spotAngle.outer);
                    break;
                }

                index++;
                if (index >= MAX_LIGHT_COUNT)
                    break;
            }
        }

        c.material->shader->set("light_count", index);
        c.mesh->draw();
    }

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json &config) {
        // First, we store the window size for later use
        this->windowSize = windowSize;

        // Then we check if there is a sky texture in the configuration
        if (config.contains("sky")) {
            // First, we create a sphere which will be used to draw the sky
            this->skySphere = mesh_utils::sphere(glm::ivec2(16, 16));
            const nlohmann::json skyData = config["sky"].get<nlohmann::json>();
            // We can draw the sky using the same shader used to draw textured objects
            ShaderProgram *skyShader = new ShaderProgram();
            skyShader->attach(skyData.value("vs", "assets/shaders/sky.vert"), GL_VERTEX_SHADER);
            skyShader->attach(skyData.value("fs", "assets/shaders/sky.frag"), GL_FRAGMENT_SHADER);
            skyShader->link();

            // TODO: (Req 10) Pick the correct pipeline state to draw the sky
            // Hints: the sky will be draw after the opaque objects so we would need depth testing
            // but which depth funtion should we pick? We will draw the sphere from the inside, so
            // what options should we pick for the face culling.
            PipelineState skyPipelineState{};
            skyPipelineState.faceCulling.enabled = true;
            skyPipelineState.faceCulling.culledFace = GL_FRONT;
            skyPipelineState.depthTesting.enabled = true;

            // Load the sky texture (note that we don't need mipmaps since we want to avoid any
            // unnecessary blurring while rendering the sky)
            std::string skyTextureFile = skyData.value("texture", "");
            Texture2D *skyTexture = texture_utils::loadImage(skyTextureFile, false);

            this->skyLight.top_color = skyData.value("top_color", glm::vec3(0.0f));
            this->skyLight.middle_color = skyData.value("middle_color", glm::vec3(0.5f));
            this->skyLight.bottom_color = skyData.value("bottom_color", glm::vec3(1.0f));
            this->skyLight.enabled = skyData.value("enabled", GL_FALSE);
            this->skyBoxExposure = skyData.value("box_exposure", 2.0f);

            // Setup a sampler for the sky
            Sampler *skySampler = new Sampler();
            skySampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_WRAP_S, GL_REPEAT);
            skySampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Combine all the aforementioned objects (except the mesh) into a material
            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->texture = skyTexture;
            this->skyMaterial->sampler = skySampler;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 1.0f;
            this->skyMaterial->transparent = false;
        }

        // Then we check if there is a postprocessing shader in the configuration
        if (config.contains("postprocess")) {
            // TODO: (Req 11) Create a framebuffer
            glGenFramebuffers(1, &postprocessFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);

            // TODO: (Req 11) Create a color and a depth texture and attach them to the framebuffer
            // Hints: The color format can be (Red, Green, Blue and Alpha components with 8 bits for
            // each channel). The depth format can be (Depth component with 24 bits).

            colorTarget = our::texture_utils::empty(GL_RGBA8, windowSize);
            depthTarget = our::texture_utils::empty(GL_DEPTH_COMPONENT24, windowSize);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   colorTarget->getOpenGLName(), 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                   depthTarget->getOpenGLName(), 0);

            // TODO: (Req 11) Unbind the framebuffer just to be safe
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Create a vertex array to use for drawing the texture
            glGenVertexArrays(1, &postProcessVertexArray);

            // Create a sampler to use for sampling the scene texture in the post processing shader
            Sampler *postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create the post processing shader
            ShaderProgram *postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config.value<std::string>("postprocess", ""),
                                      GL_FRAGMENT_SHADER);
            postprocessShader->link();

            // Create a post processing material
            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = colorTarget;
            postprocessMaterial->sampler = postprocessSampler;
            // The default options are fine but we don't need to interact with the depth buffer
            // so it is more performant to disable the depth mask
            fogPower= 0.0f;
            postprocessMaterial->pipelineState.depthMask = false;
        }
    }

    void ForwardRenderer::destroy() {
        // Delete all objects related to the sky
        if (skyMaterial) {
            delete skySphere;
            delete skyMaterial->shader;
            delete skyMaterial->texture;
            delete skyMaterial->sampler;
            delete skyMaterial;
        }
        // Delete all objects related to post processing
        if (postprocessMaterial) {
            glDeleteFramebuffers(1, &postprocessFrameBuffer);
            glDeleteVertexArrays(1, &postProcessVertexArray);
            delete colorTarget;
            delete depthTarget;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
        }
    }

    void ForwardRenderer::set_fog_power(glm::float32 power){
        fogPower = power;
    }

    void ForwardRenderer::render(World* world){
        // First of all, we search for a camera and for all the mesh renderers
        CameraComponent *camera = nullptr;
        std::vector<LightComponent *> lights;
        opaqueCommands.clear();
        transparentCommands.clear();
        for (auto entity : world->getEntities()) {
            // If we hadn't found a camera yet, we look for a camera in this entity
            if (!camera)
                camera = entity->getComponent<CameraComponent>();
            // If this entity has a mesh renderer component
            if (auto meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer) {
                // We construct a command from it
                RenderCommand command;
                command.localToWorld = meshRenderer->getOwner()->getLocalToWorldMatrix();
                command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));
                command.mesh = meshRenderer->mesh;
                command.material = meshRenderer->material;
                // if it is transparent, we add it to the transparent commands list
                if (command.material->transparent) {
                    transparentCommands.push_back(command);
                } else {
                    // Otherwise, we add it to the opaque command list
                    opaqueCommands.push_back(command);
                }
            }
            // If this entity has a light component
            if (auto light = entity->getComponent<LightComponent>(); light) {
                lights.push_back(light);
            }
        }

        // If there is no camera, we return (we cannot render without a camera)
        if (camera == nullptr)
            return;

        // TODO: (Req 9) Modify the following line such that "cameraForward" contains a vector
        // pointing the camera forward direction
        // HINT: See how you wrote the CameraComponent::getViewMatrix, it should help you solve this
        // one
        auto cameraM = camera->getOwner()->getLocalToWorldMatrix();
        glm::vec3 cameraForward = glm::vec3(cameraM * glm::vec4(0, 0, -1, 1));
        glm::vec3 cameraPosition = camera->getOwner()->localTransform.position;
        std::sort(transparentCommands.begin(), transparentCommands.end(),
                  [cameraForward](const RenderCommand &first, const RenderCommand &second) {
                      // TODO: (Req 9) Finish this function
                      // HINT: the following return should return true "first" should be drawn
                      // before "second".
                      return glm::dot(first.center, cameraForward) <
                             glm::dot(second.center, cameraForward);
                  });

        // TODO: (Req 9) Get the camera ViewProjection matrix and store it in VP
        glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();

        // TODO: (Req 9) Set the OpenGL viewport using viewportStart and viewportSize
        glViewport(0, 0, windowSize[0], windowSize[1]);

        // TODO: (Req 9) Set the clear color to black and the clear depth to 1
        glClearColor(0.0f, 0.0f, 0.0f, 0.1f);
        glClearDepth(1.0f);

        // TODO: (Req 9) Set the color mask to true and the depth mask to true (to ensure the
        // glClear will affect the framebuffer)
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        // If there is a postprocess material, bind the framebuffer
        if (postprocessMaterial) {
            // TODO: (Req 11) bind the framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, postprocessFrameBuffer);
        }

        // TODO: (Req 9) Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // TODO: (Req 9) Draw all the opaque commands
        for (RenderCommand c : opaqueCommands) {
            setupCommand(c, lights, VP, cameraPosition);
        }

        // If there is a sky material, draw the sky
        if (this->skyMaterial) {
            // TODO: (Req 10) setup the sky material
            this->skyMaterial->setup();
            // TODO: (Req 10) Create a model matrix for the sy such that it always follows the
            // camera (sky sphere center = camera position)
            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), cameraPosition);
            // TODO: (Req 10) We want the sky to be drawn behind everything (in NDC space, z=1)
            // We can acheive the is by multiplying by an extra matrix after the projection but what
            // values should we put in it?
            glm::mat4 alwaysBehindTransform =
                glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                          0.0f, 0.0f, 1.0f, 1.0f);

            this->skyMaterial->shader->set("M_transform", modelMatrix);
            this->skyMaterial->shader->set("VP_transform", alwaysBehindTransform * VP);
            this->skyMaterial->shader->set("camera_position", cameraPosition);
            this->skyMaterial->shader->set("sky_light_color.top", skyLight.top_color);
            this->skyMaterial->shader->set("sky_light_color.middle", skyLight.middle_color);
            this->skyMaterial->shader->set("sky_light_color.bottom", skyLight.bottom_color);
            this->skyMaterial->shader->set("exposure", skyBoxExposure);
            // TODO: (Req 10) draw the sky sphere
            this->skySphere->draw();
        }
        // TODO: (Req 9) Draw all the transparent commands
        for (RenderCommand c : transparentCommands) {
            setupCommand(c, lights, VP, cameraPosition);
        }

        // If there is a postprocess material, apply postprocessing
        if (postprocessMaterial) {
            // TODO: (Req 11) Return to the default framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // TODO: (Req 11) Setup the postprocess material and draw the fullscreen triangle
            postprocessMaterial->setup();

            // Bind the color texture to texture unit 0
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorTarget->getOpenGLName());
            postprocessMaterial->shader->set("color_sampler", 0);
        
            // Bind the depth texture to texture unit 1
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, depthTarget->getOpenGLName());
            postprocessMaterial->shader->set("depth_sampler", 1);
        
            // Set the inverse projection matrix
            postprocessMaterial->shader->set("inverse_projection", glm::inverse(camera->getProjectionMatrix(windowSize)));
        
            // Set fog parameters
            postprocessMaterial->shader->set("fog_color", glm::vec3(0.8f, 0.8f, 0.8f)); // Example fog color
            postprocessMaterial->shader->set("fog_power", fogPower); // Example fog power
            postprocessMaterial->shader->set("fog_exponent", 0.02f); // Example fog exponent
            

            glBindVertexArray(postProcessVertexArray);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
        }
    }

}