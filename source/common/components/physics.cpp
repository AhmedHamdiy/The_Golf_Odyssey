#include "physics.hpp"
#include "../ecs/entity.hpp"
#include "../deserialize-utils.hpp"

namespace our {
    void PhysicsComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        mass = data.value("mass", 0.0f);
        std::string groupStr = data.value("group", "static");
        if(groupStr == "kinematic"){
            group = collisionType::KINEMATIC;
        } else if(groupStr == "dynamic" || mass != 0.0f){
            group = collisionType::DYNAMIC;
        } else {
            group = collisionType::STATIC;
        }
    }
}