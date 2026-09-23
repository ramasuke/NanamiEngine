#pragma once
#include <cmath>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Namespace/EngineNamespace.h"
#include "../glm/gtx/quaternion.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    // 水平面上で前方向(-Z)から direction への角度(deg, -180..180)。右が正
    // NOTE: DxLib は左手系なので、-Z を向いたときの右は -X
    inline bool SignedHorizontalAngleDeg(const GameObject::Transform& transform, glm::vec3 direction, float& outDeg)
    {
        direction.y = 0.0f;
        glm::vec3 forward = transform.GetWorldRot() * glm::vec3(0, 0, -1);
        forward.y = 0.0f;
        if (glm::length2(direction) < 1e-6f || glm::length2(forward) < 1e-6f)
            return false;

        forward   = glm::normalize(forward);
        direction = glm::normalize(direction);
        const float crossY = forward.z * direction.x - forward.x * direction.z;
        outDeg = glm::degrees(std::atan2(crossY,glm::dot(forward, direction)));
        return true;
    }

    // 水平面上で前方向(-Z)を direction へ最大 rotateSpeedDeg (deg/sec) で回す
    inline void RotateTowardsHorizontal(GameObject::Transform& transform, glm::vec3 direction, const float rotateSpeedDeg)
    {
        direction.y = 0.0f;
        if (glm::length2(direction) < 1e-6f)
            return;
        direction = glm::normalize(direction);

        glm::vec3 forward = transform.GetWorldRot() * glm::vec3(0, 0, -1);
        forward.y = 0.0f;
        if (glm::length2(forward) < 1e-6f)
            return;
        forward = glm::normalize(forward);

        const float dot      = glm::clamp(glm::dot(forward, direction), -1.0f, 1.0f);
        const float angleRad = std::acos(dot);
        if (angleRad <= 1e-4f)
            return;

        const glm::quat currentRot = transform.GetWorldRot();
        const glm::quat deltaRot   = dot < -0.9999f
            ? glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0))
            : glm::rotation(forward, direction);

        const float maxStep = glm::radians(rotateSpeedDeg) * Time::DeltaTime();
        const float t       = glm::min(1.0f, maxStep / angleRad);
        transform.SetWorldRot(glm::slerp(currentRot, deltaRot * currentRot, t));
    }
}
