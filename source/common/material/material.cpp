#include "material.hpp"

#include "../asset-loader.hpp"
#include "deserialize-utils.hpp"

namespace our {

    // This function should setup the pipeline state and set the shader to be used
    void Material::setup() const {
        //TODO: (Req 7) Write this function
        pipelineState.setup();
        shader->use();
    }

    // This function read the material data from a json object
    void Material::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;

        if(data.contains("pipelineState")){
            pipelineState.deserialize(data["pipelineState"]);
        }
        shader = AssetLoader<ShaderProgram>::get(data["shader"].get<std::string>());
        transparent = data.value("transparent", false);
    }

    // This function should call the setup of its parent and
    // set the "tint" uniform to the value in the member variable tint 
    void TintedMaterial::setup() const {
        //TODO: (Req 7) Write this function
        Material::setup();
        shader->set("tint", tint);
    }

    // This function read the material data from a json object
    void TintedMaterial::deserialize(const nlohmann::json& data){
        Material::deserialize(data);
        if(!data.is_object()) return;
        tint = data.value("tint", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    // This function should call the setup of its parent and
    // set the "alphaThreshold" uniform to the value in the member variable alphaThreshold
    // Then it should bind the texture and sampler to a texture unit and send the unit number to the uniform variable "tex" 
    void TexturedMaterial::setup() const {
        //TODO: (Req 7) Write this function
        TintedMaterial::setup();
        shader->set("alphaThreshold", alphaThreshold);
        glActiveTexture(GL_TEXTURE0);
        texture->bind();
        if (sampler) sampler->bind(0);
        shader->set("tex", 0);
        shader->set("tiling", tiling);
    }

    // This function read the material data from a json object
    void TexturedMaterial::deserialize(const nlohmann::json& data){
        TintedMaterial::deserialize(data);
        if(!data.is_object()) return;
        alphaThreshold = data.value("alphaThreshold", 0.0f);
        texture = AssetLoader<Texture2D>::get(data.value("texture", ""));
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
        if (data.contains("tiling") && data["tiling"].is_array())
            tiling = glm::vec2(data["tiling"][0], data["tiling"][1]);
    }

    void LitMaterial::setup() const {
        Material::setup();
        shader->set("material.albedo_tint", albedo_tint);
        shader->set("material.specular_tint", specular_tint);
        shader->set("material.emissive_tint", emissive_tint);
        shader->set("material.roughness_range", roughness_range);
        if (albedo_texture) {
            glActiveTexture(GL_TEXTURE0);
            albedo_texture->bind();
            shader->set("material.albedo_tex", 0);
        }
        if (specular_texture) {
            glActiveTexture(GL_TEXTURE1);
            specular_texture->bind();
            shader->set("material.specular_tex", 1);
        }
        if (ambient_occlusion_texture) {
            glActiveTexture(GL_TEXTURE2);
            ambient_occlusion_texture->bind();
            shader->set("material.ambient_occlusion_tex", 2);
        }
        if (roughness_texture) {
            glActiveTexture(GL_TEXTURE3);
            roughness_texture->bind();
            shader->set("material.roughness_tex", 3);
        }
        if (emissive_texture) {
            glActiveTexture(GL_TEXTURE4);
            emissive_texture->bind();
            shader->set("material.emissive_tex", 4);
        }
        if (sampler) {
            for (GLuint tex_unit = 0; tex_unit < 5; ++tex_unit)
                sampler->bind(tex_unit);
        }
    }

    void LitMaterial::deserialize(const nlohmann::json& data){
        Material::deserialize(data);
        if(!data.is_object()) return;
        sampler = AssetLoader<Sampler>::get(data.value("sampler", ""));
        albedo_texture = AssetLoader<Texture2D>::get(data.value("albedo_texture", ""));
        specular_texture = AssetLoader<Texture2D>::get(data.value("specular_texture", ""));
        emissive_texture = AssetLoader<Texture2D>::get(data.value("emissive_texture", ""));
        roughness_texture = AssetLoader<Texture2D>::get(data.value("roughness_texture", ""));
        ambient_occlusion_texture = AssetLoader<Texture2D>::get(data.value("ambient_occlusion_texture", ""));
        albedo_tint = data.value("albedo_tint", glm::vec3(1.0f, 1.0f, 1.0f));
        specular_tint = data.value("specular_tint", glm::vec3(1.0f, 1.0f, 1.0f));
        emissive_tint = data.value("emissive_tint", glm::vec3(1.0f, 1.0f, 1.0f));
        roughness_range = data.value("roughness_range", glm::vec2(0.0f, 1.0f));
    }
}