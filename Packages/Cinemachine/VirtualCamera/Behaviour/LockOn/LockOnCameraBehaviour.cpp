#include "LockOnCameraBehaviour.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../Engine/Module/Physics/RaycastHit/Engine_Physics_RaycastHit.h"
#include "../../../../../Engine/Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"

namespace NanamiEngine::CineMachine::Behaviour
{
    void LockOnCameraBehaviour::SetFollowTarget(const std::shared_ptr<GameObject::IGameObject>& followTarget)
    {
        followTarget_ = followTarget;
        RequireComponent<VirtualCameraFollowBehaviour>()->SetTarget(followTarget);
        RequireComponent<VirtualCameraLookAtBehaviour>()->SetTarget(followTarget);
    }

    void LockOnCameraBehaviour::SetLockOnTarget(const std::shared_ptr<GameObject::IGameObject>& lockOnTarget)
    {
        lockOnTarget_ = lockOnTarget;
    }

    void LockOnCameraBehaviour::ClearLockOnTarget()
    {
        lockOnTarget_.reset();
    }

    void LockOnCameraBehaviour::OnAwake()
    {
        follow_ = RequireComponent<VirtualCameraFollowBehaviour>();
        lookAt_ = RequireComponent<VirtualCameraLookAtBehaviour>();
    }

    void LockOnCameraBehaviour::OnUpdate()
    {
        const auto lockOnTarget = lockOnTarget_.lock();
        if (!lockOnTarget)
            return;

        UpdateFollowBehaviour(lockOnTarget);
    }

    void LockOnCameraBehaviour::UpdateFollowBehaviour(const std::shared_ptr<GameObject::IGameObject>& lockOnTarget) const
    {
        if (!followTarget_)
            return;

        const glm::vec3 playerPos = followTarget_->Transform().GetWorldPos();
        const glm::vec3 targetPos = lockOnTarget->Transform().GetWorldPos();

        glm::vec3 flatToTarget = glm::vec3(targetPos.x - playerPos.x, 0.0f, targetPos.z - playerPos.z);
        if (glm::dot(flatToTarget, flatToTarget) < 0.0001f)
            flatToTarget = glm::vec3(0.0f, 0.0f, 1.0f);
        flatToTarget = glm::normalize(flatToTarget);

        // プレイヤーとロックオン対象を結ぶ横方向のベクトル。カメラをここへ寄せて両者をフレームに収める
        const glm::vec3 sideDir = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), flatToTarget));

        const glm::vec3 desiredOffset = -flatToTarget * distance_ + glm::vec3(0.0f, height_, 0.0f) + sideDir * sideOffset_;

        // 壁などにめり込まないよう、Playerからカメラへrayを飛ばして位置を補正する
        const glm::vec3 adjustedOffset = ResolveCameraCollision(playerPos, desiredOffset);

        follow_->followOffset_ = adjustedOffset;

        // プレイヤーとロックオン対象の中間点を見るよう、LookAt対象(プレイヤー)へのオフセットで調整する
        lookAt_->SetOffsetPos((targetPos - playerPos) * 0.5f + glm::vec3(0.0f, lookAtHeightOffset_, 0.0f));
    }

    glm::vec3 LockOnCameraBehaviour::ResolveCameraCollision(const glm::vec3& originPos, const glm::vec3& desiredOffset) const
    {
        const float distance = glm::length(desiredOffset);
        if (distance <= 0.0f)
            return desiredOffset;

        const glm::vec3 direction = desiredOffset / distance;

        Module::Physics::LayerMask mask = Module::Physics::CreateLayerMask();
        Module::Physics::AddLayer(mask, Module::Physics::Layer::Default);

        const Module::Physics::RaycastHit hit = Module::Physics::Raycast(originPos, direction, distance, mask);
        if (!hit.Hit())
            return desiredOffset;

        // 障害物の少し手前にカメラを配置する
        const float hitDistance      = glm::length(hit.Position() - originPos);
        const float adjustedDistance = std::max(0.0f, hitDistance - collisionBuffer_);

        return direction * adjustedDistance;
    }

    void LockOnCameraBehaviour::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("distance_", distance_);
        ImGuiHelper::OnDrawInputField("height_", height_);
        ImGuiHelper::OnDrawInputField("sideOffset_", sideOffset_);
        ImGuiHelper::OnDrawInputField("lookAtHeightOffset_", lookAtHeightOffset_);
        ImGuiHelper::OnDrawInputField("collisionBuffer_", collisionBuffer_);
        ImGuiHelper::OnDrawInputField("followTarget_", followTarget_);
        ImGuiHelper::OnDrawInputField("follow_", follow_);
        ImGuiHelper::OnDrawInputField("lookAt_", lookAt_);
    }
}
