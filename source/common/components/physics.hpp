#pragma once

#include "../ecs/component.hpp"
#include "../mesh/vertex.hpp"
#include <glm/glm.hpp>

namespace our {
    class PhysicsComponent : public Component {
    public:
        float mass;

        static std::string getID() { return "Physics"; }
        void deserialize(const nlohmann::json& data) override;
    };
}