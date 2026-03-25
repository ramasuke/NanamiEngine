#include "Friendly_Behaviour_Action_MoveForRoute.h"

#include "../../../../../../../../../../Engine/Module/Component/Collider/ICollider.h"
#include "../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../../../../Engine/Module/Physics/Physics_.h"
#include "gtc/quaternion.hpp"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::MoveForRoute::DoTick(const TickContext& context)
    {
        const JPH::BodyID& bodyId = context.NpcCollider().BodyId();

        const auto& route = moveRoute_->Get();
        if (route.empty())
            return TickStatus::Failure;

        const glm::vec3 currentPos = context.NpcTransform().GetWorldPos();
        const glm::vec3 targetPos  = route[currentRouteIndex_];

        glm::vec3 toTarget = targetPos - currentPos;
        toTarget.y = 0.0f;

        const float distance = glm::length(toTarget);

        // 到達判定
        if (distance <= ARRIVE_THRESHOLD)
        {
            currentRouteIndex_++;

            if (currentRouteIndex_ >= static_cast<int>(route.size()))
            {
                Physics::SetLinearVelocity(bodyId, glm::vec3(0.0f));
                currentRouteIndex_ = 0;
                return TickStatus::Success;
            }

            return TickStatus::Running;
        }

        // ===== 回転処理（※一切変更なし） =====
        const glm::vec3 moveDirection = toTarget / distance;
        const glm::quat currentRot    = context.NpcTransform().GetWorldRot();
        const float     currentYaw    = glm::eulerAngles(currentRot).y;
        const float     targetYaw     = std::atan2(-moveDirection.x, -moveDirection.z);

        float deltaYaw = targetYaw - currentYaw;
        deltaYaw = std::atan2(std::sin(deltaYaw), std::cos(deltaYaw));
        const float maxStep = turnRotateSpeed_ * Time::DeltaTime();
        const float yawStep = glm::clamp(deltaYaw, -maxStep, maxStep);
        context.NpcTransform().SetWorldRot(
            glm::vec3(0.0f, currentYaw + yawStep, 0.0f)
        );

        // ===== 移動処理（目的地ベース） =====
        const glm::vec3 moveDir = moveDirection; // 正規化済み

        glm::vec3 velocity = Physics::GetLinearVelocity(bodyId);
        velocity.x = moveDir.x * moveSpeed_;
        velocity.z = moveDir.z * moveSpeed_;

        Physics::SetLinearVelocity(bodyId, velocity);

        return TickStatus::Running;
    }

    void Action::MoveForRoute::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("moveRoute_", moveRoute_);
        ImGuiHelper::OnDrawInputField("moveSpeed_", moveSpeed_);
        ImGuiHelper::OnDrawInputField("turnRotateSpeed_", turnRotateSpeed_);
        ImGuiHelper::OnDrawInputField("moveAnimationParamNumber_", moveAnimationParamNumber_);
    }
}
