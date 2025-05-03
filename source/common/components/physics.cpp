#include "physics.hpp"
#include "../ecs/entity.hpp"
#include "../deserialize-utils.hpp"

namespace our {
    void PhysicsComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        mass = data.value("mass", 0.0f);
    }
}