#include "light.hpp"
#include "../deserialize-utils.hpp"

namespace our {
    void LightComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object() || !data["lights"].is_array()) return;

        for(auto& light: data["lights"]) {
            if(!light.is_object()) continue;
            Light new_light;

            if (light.value("type", "DIRECTIONAL") == "DIRECTIONAL") new_light.type = LightType::DIRECTIONAL;
            else if (light.value("type", "DIRECTIONAL") == "POINT") new_light.type = LightType::POINT;
            else new_light.type = LightType::SPOT;
    
            new_light.enabled = light.value("enabled", GL_FALSE);
            new_light.color = light.value("color", glm::vec3(1.0f));

            if (light.contains("attenuation")) {
                new_light.attenuation.constant = light["attenuation"].value("constant", 0.0f);
                new_light.attenuation.linear = light["attenuation"].value("linear", 0.0f);
                new_light.attenuation.quadratic = light["attenuation"].value("quadratic", 1.0f);
            }
            else {
                new_light.attenuation = { 0.0f, 0.0f, 1.0f };
            }

            if (light.contains("spotAngle")) {
                new_light.spotAngle.inner = glm::radians(light["spotAngle"].value("inner", 45.0f));
                new_light.spotAngle.outer = glm::radians(light["spotAngle"].value("outer", 90.0f));
            }
            else {
                new_light.spotAngle = { 45.0f, 90.0f };
            }

            lights.push_back(new_light);
        }
    }
}