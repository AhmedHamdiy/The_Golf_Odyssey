#pragma once

#include "../ecs/component.hpp"

#include <glm/glm.hpp>

namespace our {
    enum class LightType {
        DIRECTIONAL,
        POINT,
        SPOT
    };

    struct SpotAngle{
        float inner, outer;
    };

    struct Attenuation {
        float constant, linear, quadratic;
    };

    struct Light {
        LightType type;
        glm::vec3 color;
        Attenuation attenuation;
        SpotAngle spotAngle;
        bool enabled;
    };

    class LightComponent : public Component {
    public:
        std::vector<Light> lights;
        
        // The ID of this component type is "Light"
        static std::string getID() { return "Lights"; }

        // Reads light data from the given json object
        void deserialize(const nlohmann::json& data) override;
    };

}