#include "PlayerAvatarLockOn.h"

#include <algorithm>
#include <optional>
#include <vector>

#include "geometric.hpp"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "Engine/Module/Physics/Component/Collider/Engine_Physics_ICollider.h"
#include "Packages/Cinemachine/Brain/CinemachineCameraBrain.h"
#include "../../../../GamePlay/PlayerAvatar/LockOnDetectionArea/LockOnDetectionArea.h"
#include "../CameraGroup/PlayerAvatarCameraGroupBase.h"
#include "ILockOnTarget.h"

namespace GameCore::PlayerAvatar
{
    namespace
    {
        struct AimCandidate
        {
            std::shared_ptr<GameObject::IGameObject> target;
            float screenX;
        };
    }

    LockOnController::LockOnController(PlayerAvatarCameraGroupBase& cameraGroup,
                                       const GamePlay::PlayerAvatar::LockOnDetectionArea& detectionArea)
        : cameraGroup_  (cameraGroup  )
        , detectionArea_(detectionArea)
    {
    }

    bool LockOnController::Update(const glm::vec3& playerPos, const bool isLockOnPressed, const int switchDirection) const
    {
        if (cameraGroup_.IsLockedOn() && !IsTargetInRange())
            cameraGroup_.ReleaseLockOn();

        const auto nearestTarget = cameraGroup_.IsLockedOn() ? nullptr : FindNearestTarget(playerPos);
        cameraGroup_.SetLockOnCandidate(nearestTarget);

        if (cameraGroup_.IsLockedOn() && switchDirection != 0)
            SwitchTarget(switchDirection);

        if (!isLockOnPressed)
            return false;

        if (cameraGroup_.IsLockedOn())
        {
            cameraGroup_.ReleaseLockOn();
            return false;
        }

        if (!nearestTarget)
            return false;

        cameraGroup_.EngageLockOn(nearestTarget);
        return true;
    }

    bool LockOnController::IsTargetInRange() const
    {
        const auto currentTarget = cameraGroup_.LockOnTarget().lock();
        if (!currentTarget)
            return false;

        for (const auto& candidate : detectionArea_.Candidates())
            if (candidate.lock() == currentTarget)
                return HasLineOfSight(currentTarget); // 索敵範囲内でも遮蔽されたら解除

        return false; // 索敵範囲外に出た
    }

    std::shared_ptr<GameObject::IGameObject> LockOnController::FindNearestTarget(const glm::vec3& playerPos) const
    {
        std::shared_ptr<GameObject::IGameObject> nearestTarget;
        float nearestDistanceSq = -1.0f;

        for (const auto& weakCandidate : detectionArea_.Candidates())
        {
            const auto candidate = weakCandidate.lock();
            if (!candidate)
                continue;
            if (!HasLineOfSight(candidate))
                continue;

            const glm::vec3 diff = candidate->Transform().GetWorldPos() - playerPos;
            const float distanceSq = glm::dot(diff, diff);
            if (nearestDistanceSq < 0.0f || distanceSq < nearestDistanceSq)
            {
                nearestDistanceSq = distanceSq;
                nearestTarget = candidate;
            }
        }
        return nearestTarget;
    }

    void LockOnController::SwitchTarget(const int direction) const
    {
        const auto currentTarget = cameraGroup_.LockOnTarget().lock();
        const auto* brain        = CineMachine::CinemachineCameraBrain::Instance();
        if (!currentTarget || !brain || direction == 0)
            return;

        const glm::vec3 cameraPos = brain->Transform().GetWorldPos();
        const glm::quat cameraRot = brain->Transform().GetWorldRot();
        const glm::vec3 forward   = cameraRot * glm::vec3(0.0f, 0.0f, 1.0f);
        const glm::vec3 right     = cameraRot * glm::vec3(1.0f, 0.0f, 0.0f);

        // 画面の横位置。Update 中は DxLib のカメラがエディタの Scene ビューのままのことがあるので、Brain の姿勢から求める
        const auto screenXOf = [&](const glm::vec3& point) -> std::optional<float>
        {
            const glm::vec3 diff = point - cameraPos;
            const float depth = glm::dot(diff, forward);
            if (depth <= 0.0f)
                return std::nullopt;
            return glm::dot(diff, right) / depth;
        };

        std::vector<AimCandidate> candidates;
        for (const auto& weakCandidate : detectionArea_.Candidates())
        {
            const auto target = weakCandidate.lock();
            if (!target || target == currentTarget || !HasLineOfSight(target))
                continue;

            if (const auto screenX = screenXOf(ILockOnTarget::PositionOf(*target)))
                candidates.push_back({ target, *screenX });
        }
        if (candidates.empty())
            return;

        // 向かう側で一番近い点。無ければ(端にいる、今の点が画面外)反対側の端へ回る
        const AimCandidate* next = nullptr;
        if (const auto currentX = screenXOf(ILockOnTarget::PositionOf(*currentTarget)))
        {
            for (const auto& candidate : candidates)
            {
                const float offset = (candidate.screenX - *currentX) * static_cast<float>(direction);
                if (offset > 0.0f && (!next || offset < (next->screenX - *currentX) * static_cast<float>(direction)))
                    next = &candidate;
            }
        }
        if (!next)
        {
            const auto byScreenX = [](const AimCandidate& a, const AimCandidate& b) { return a.screenX < b.screenX; };
            next = direction > 0
                ? &*std::min_element(candidates.begin(), candidates.end(), byScreenX)
                : &*std::max_element(candidates.begin(), candidates.end(), byScreenX);
        }

        cameraGroup_.EngageLockOn(next->target);
    }

    bool LockOnController::HasLineOfSight(const std::shared_ptr<GameObject::IGameObject>& target)
    {
        const auto targetCollider = target->Components().Catch<Physics::ICollider>().lock();
        const glm::vec3 targetPos = targetCollider
            ? targetCollider->CenterOfMassPosition().value_or(target->Transform().GetWorldPos())
            : target->Transform().GetWorldPos();
        return HasLineOfSight(targetPos, *target);
    }

    bool LockOnController::HasLineOfSight(const glm::vec3& point, const GameObject::IGameObject& target)
    {
        const glm::vec3 origin = CineMachine::CinemachineCameraBrain::Instance()->Transform().GetWorldPos();

        const glm::vec3 diff = point - origin;
        const float distance = glm::length(diff);
        if (distance <= 0.0f)
            return true;

        // 敵は遮蔽物に含めない
        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        const auto hit = Physics::Raycast(origin, diff, distance, mask);
        return !hit.Hit() || &hit.HitObject() == &target;
    }
}
