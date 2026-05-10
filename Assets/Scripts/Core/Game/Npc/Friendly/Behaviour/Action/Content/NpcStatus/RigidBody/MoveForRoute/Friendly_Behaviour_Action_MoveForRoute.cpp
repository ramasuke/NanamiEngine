#include "Friendly_Behaviour_Action_MoveForRoute.h"

#include "../../../../../../../../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ICollider.h"
#include "../../../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "gtc/quaternion.hpp"

namespace
{
    void RotateTowardsDirY(
        GameObject::Transform& transform,
        const glm::vec3& direction,
        const float rotateSpeed)
    {
        glm::vec3 targetForward(direction.x, 0.0f, direction.z);
        if (glm::length2(targetForward) < 0.0001f)
            return;

        targetForward = glm::normalize(targetForward);

        glm::vec3 currentForward =
            transform.GetWorldRot() * glm::vec3(0, 0, -1);

        currentForward.y = 0.0f;

        if (glm::length2(currentForward) < 0.0001f)
            return;

        currentForward = glm::normalize(currentForward);

        const float dot = glm::dot(currentForward, targetForward);
        const glm::quat currentRot = transform.GetWorldRot();

        if (dot > 0.9999f)
            return;

        glm::quat targetRot;

        if (dot < -0.9999f)
        {
            // 180度反転対策
            targetRot =
                glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0)) *
                currentRot;
        }
        else
        {
            targetRot =
                glm::rotation(currentForward, targetForward) *
                currentRot;
        }

        const float angleDiff = glm::angle(targetRot * glm::inverse(currentRot));
        const float maxStep   = rotateSpeed * Time::DeltaTime();

        const float t = glm::min(1.0f, maxStep / angleDiff);

        transform.SetWorldRot(glm::slerp(currentRot, targetRot, t));
    }
}

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

        // 回転
        RotateTowardsDirY(
            context.NpcTransform(),
            toTarget,
            turnRotateSpeed_
        );

        // 移動
        const glm::vec3 moveDir = glm::normalize(toTarget);

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
    }
}