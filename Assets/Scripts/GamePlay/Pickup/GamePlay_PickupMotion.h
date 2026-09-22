#pragma once
#include "vec3.hpp"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class ComponentGroup;
}

namespace GamePlay::Pickup
{
    // 島の外へ落ちた拾い物を落とし続けないよう、出た高さからこれだけ落ちたら消す
    constexpr float PICKUP_FALL_OUT_DEPTH = 500.0f;

    /** @brief 拾い物の RigidBody を sideDirection 側へ、横速度をランダムにして跳ね上げる */
    void LaunchPickup(GameObject::ComponentGroup& components,
                      const glm::vec3& sideDirection,
                      float upSpeed,
                      float sideSpeedMin,
                      float sideSpeedMax);
}
