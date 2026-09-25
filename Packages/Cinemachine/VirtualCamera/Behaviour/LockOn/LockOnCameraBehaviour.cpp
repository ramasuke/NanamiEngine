#include "LockOnCameraBehaviour.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "DxLib.h"
#include "../../../Brain/CinemachineCameraBrain.h"
#include "../IVirtualCameraTarget.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ICollider.h"
#include "../../../../../Engine/Module/Physics/RaycastHit/Engine_Physics_RaycastHit.h"
#include "../../../../../Engine/Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"
#include "../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace
{
    std::pair<glm::vec3, glm::vec3> WorldBoundsOf(NanamiEngine::Module::GameObject::IGameObject& object, const float fallbackRadius)
    {
        const auto collider = object.Components().Catch<NanamiEngine::Module::Physics::ICollider>().lock();
        if (const auto bounds = collider ? collider->WorldBounds() : std::nullopt)
            return *bounds;

        const glm::vec3 center = object.Transform().GetWorldPos();
        return { center - glm::vec3(fallbackRadius), center + glm::vec3(fallbackRadius) };
    }

    glm::vec3 BoundsCorner(const glm::vec3& min, const glm::vec3& max, const int index)
    {
        return { index & 1 ? max.x : min.x, index & 2 ? max.y : min.y, index & 4 ? max.z : min.z };
    }
}

namespace NanamiEngine::CineMachine::Behaviour
{
    void LockOnCameraBehaviour::SetFollowTarget(const std::shared_ptr<GameObject::IGameObject>& followTarget)
    {
        followTarget_ = followTarget;
        RequireComponent<VirtualCameraFollowBehaviour>()->SetTarget(followTarget);
        RequireComponent<VirtualCameraLookAtBehaviour>()->SetTarget(followTarget);
    }

    void LockOnCameraBehaviour::SetLockOnTarget(const std::shared_ptr<GameObject::IGameObject>& lockOnTarget,
                                                const std::shared_ptr<GameObject::IGameObject>& aim)
    {
        lockOnTarget_ = lockOnTarget;
        lockOnAim_ = aim;
    }

    void LockOnCameraBehaviour::ClearLockOnTarget()
    {
        lockOnTarget_.reset();
        lockOnAim_.reset();
    }

    void LockOnCameraBehaviour::OnAwake()
    {
        follow_ = RequireComponent<VirtualCameraFollowBehaviour>();
        lookAt_ = RequireComponent<VirtualCameraLookAtBehaviour>();
        virtualCamera_ = Components().Catch<CineMachineVirtualCamera>();
    }

    void LockOnCameraBehaviour::OnCameraUpdate()
    {
        const auto lockOnTarget = lockOnTarget_.lock();
        if (!lockOnTarget)
            return;

        UpdateFraming(lockOnTarget);
    }

    void LockOnCameraBehaviour::UpdateFraming(const std::shared_ptr<GameObject::IGameObject>& lockOnTarget)
    {
        const auto followTarget = followTarget_.get();
        if (!followTarget)
            return;

        constexpr auto worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        // Follow/LookAtはこの位置にオフセットを足すので、同じ基準点を使う
        const glm::vec3 playerPos = IVirtualCameraTarget::PositionOf(*followTarget);
        const glm::vec3 targetPos = lockOnTarget->Transform().GetWorldPos();

        const glm::vec3 flatToTarget(targetPos.x - playerPos.x, 0.0f, targetPos.z - playerPos.z);
        if (glm::dot(flatToTarget, flatToTarget) >= 0.0001f)
            lastFlatDir_ = glm::normalize(flatToTarget);

        // right/up は VirtualCameraLookAtBehaviour が作る姿勢と同じ外積の順にそろえる
        const float pitch = glm::radians(pitchAngle_deg_);
        const glm::vec3 forward = glm::normalize(lastFlatDir_ * std::cos(pitch) - worldUp * std::sin(pitch));
        const glm::vec3 right   = glm::normalize(glm::cross(worldUp, forward));
        const glm::vec3 up      = glm::cross(forward, right);

        const auto [playerMin, playerMax] = WorldBoundsOf(*followTarget, fallbackBoundsRadius_);
        const auto [targetMin, targetMax] = WorldBoundsOf(*lockOnTarget, fallbackBoundsRadius_);

        std::array<glm::vec3, 17> points;
        for (int i = 0; i < 8; ++i)
        {
            points[i]     = BoundsCorner(playerMin, playerMax, i);
            points[i + 8] = BoundsCorner(targetMin, targetMax, i);
        }
        const auto aim = lockOnAim_.lock();
        points[16] = ILockOnCameraTarget::PositionOf(aim ? *aim : *lockOnTarget);

        // 画面の縦横方向に投影した範囲の中心を注視点にする
        glm::vec2 projectedMin(std::numeric_limits<float>::max());
        glm::vec2 projectedMax(std::numeric_limits<float>::lowest());
        for (const glm::vec3& point : points)
        {
            const glm::vec2 projected(glm::dot(point - playerPos, right), glm::dot(point - playerPos, up));
            projectedMin = glm::min(projectedMin, projected);
            projectedMax = glm::max(projectedMax, projected);
        }
        const glm::vec2 projectedCenter = (projectedMin + projectedMax) * 0.5f;
        const glm::vec3 lookAtPos = playerPos + right * projectedCenter.x + up * projectedCenter.y;

        // 全ての点が画角(余白込み)に入る、注視点からの最小距離を求める
        float requiredDistance = minDistance_;
        if (const auto* brain = CinemachineCameraBrain::Instance())
        {
            int screenWidth, screenHeight;
            GetScreenState(&screenWidth, &screenHeight, nullptr);
            const float aspectRatio = screenHeight > 0 ? static_cast<float>(screenWidth) / static_cast<float>(screenHeight) : 1.0f;

            const auto  virtualCamera = virtualCamera_.lock();
            const float fov = virtualCamera ? virtualCamera->Fov() : brain->GetFov();

            const float margin = std::clamp(framingMargin_, 0.0f, 0.9f);
            const float tanHalfFovY = std::tan(glm::radians(fov) * 0.5f) * (1.0f - margin);
            const float tanHalfFovX = tanHalfFovY * aspectRatio;

            for (const glm::vec3& point : points)
            {
                const glm::vec3 fromLookAt = point - lookAtPos;
                const float depth = glm::dot(fromLookAt, forward);
                requiredDistance = std::max(requiredDistance, std::abs(glm::dot(fromLookAt, right)) / tanHalfFovX - depth);
                requiredDistance = std::max(requiredDistance, std::abs(glm::dot(fromLookAt, up)) / tanHalfFovY - depth);
            }
        }
        const float distance = std::min(requiredDistance, maxDistance_);

        const glm::vec3 cameraPos = lookAtPos - forward * distance;

        // 壁などにめり込まないよう、Playerからカメラへrayを飛ばして位置を補正する
        follow_->followOffset_ = ResolveCameraCollision(playerPos, cameraPos - playerPos);
        lookAt_->SetOffsetPos(lookAtPos - playerPos);
    }

    glm::vec3 LockOnCameraBehaviour::ResolveCameraCollision(const glm::vec3& originPos, const glm::vec3& desiredOffset) const
    {
        const float distance = glm::length(desiredOffset);
        if (distance <= 0.0f)
            return desiredOffset;

        const glm::vec3 direction = desiredOffset / distance;

        Module::Physics::LayerMask mask = Module::Physics::CreateLayerMask();
        Module::Physics::AddLayer(mask, Module::Physics::Layer::Default);

        // 位置を決める。
        Module::Physics::RaycastHit hit = Module::Physics::SphereCast(originPos, collisionRadius_, direction, distance, mask);
        if (hit.Hit() && hit.Distance() <= 0.0f)
        {
            // 始点の時点で球が既に壁に重なっている場合、Rayにフォールバック
            hit = Module::Physics::Raycast(originPos, direction, distance, mask);
        }
        if (!hit.Hit())
            return desiredOffset;

        // 障害物の少し手前にカメラを配置する
        const float adjustedDistance = std::max(0.0f, hit.Distance() - collisionBuffer_);

        return direction * adjustedDistance;
    }

    void LockOnCameraBehaviour::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("isImmediateApply_", isImmediateApply_);
        ImGuiHelper::OnDrawInputField("pitchAngle_deg_", pitchAngle_deg_);
        ImGuiHelper::OnDrawInputField("minDistance_", minDistance_);
        ImGuiHelper::OnDrawInputField("maxDistance_", maxDistance_);
        ImGuiHelper::OnDrawInputField("framingMargin_", framingMargin_);
        ImGuiHelper::OnDrawInputField("fallbackBoundsRadius_", fallbackBoundsRadius_);
        ImGuiHelper::OnDrawInputField("collisionBuffer_", collisionBuffer_);
        ImGuiHelper::OnDrawInputField("collisionRadius_", collisionRadius_);
        ImGuiHelper::OnDrawInputField("followTarget_", followTarget_);
        ImGuiHelper::OnDrawInputField("follow_", follow_);
        ImGuiHelper::OnDrawInputField("lookAt_", lookAt_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour);
NANAMI_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IAwakable, NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour);
NANAMI_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::CineMachine::IVirtualCameraBehaviour, NanamiEngine::CineMachine::Behaviour::LockOnCameraBehaviour);
#pragma endregion
