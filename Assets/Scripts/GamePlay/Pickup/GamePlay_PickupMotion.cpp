#include "GamePlay_PickupMotion.h"

#include <algorithm>
#include <random>

#include "../../../../Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"

namespace GamePlay::Pickup
{
    void LaunchPickup(GameObject::ComponentGroup& components,
                      const glm::vec3& sideDirection,
                      const float upSpeed,
                      const float sideSpeedMin,
                      const float sideSpeedMax)
    {
        static std::mt19937 random{ std::random_device{}() };
        const float minSideSpeed = (std::min)(sideSpeedMin, sideSpeedMax);
        const float maxSideSpeed = (std::max)(sideSpeedMin, sideSpeedMax);
        std::uniform_real_distribution<float> sideSpeed(minSideSpeed, maxSideSpeed);

        if (const auto rigidBody = components.Catch<Component::RigidBody>().lock())
            rigidBody->SetLinearVelocity(sideDirection * sideSpeed(random) + glm::vec3(0.0f, upSpeed, 0.0f));
    }
}
