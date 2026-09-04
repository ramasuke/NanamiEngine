#include "Enemy_Behaviour_Action_ChasePlayerForPathFinding.h"

#include <algorithm>
#include <cmath>

#include "../../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "../glm/gtx/quaternion.hpp"
#include "../glm/gtx/vector_angle.hpp"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ChasePlayerForPathFinding::DoTick(const TickContext& context)
    {
        const auto grid = heightGridMap_.get();
        if (!grid)
            return TickStatus::Failure;

        const glm::vec3 selfPos = context.EnemyTransform().GetWorldPos();

        std::vector<glm::vec3> playerPositions;
        for (const auto& player : context.AllPlayer())
            playerPositions.push_back(player.lock()->PlayerTransform().GetWorldPos());

        if (playerPositions.empty())
            return TickStatus::Failure;

        pathFinder_.Tick(grid, selfPos, playerPositions,
                         maxPathCellRange_, maxClimbAngleDeg_, searchIntervalSec_);

        if (!pathFinder_.HasPath() || pathFinder_.Path().empty())
            return TickStatus::Running;

        // 到達済みウェイポイントを読み飛ばして前進
        const glm::vec2 cellSize        = grid->CellSize();
        const float     halfCell        = 0.5f * (std::min)(std::abs(cellSize.x), std::abs(cellSize.y));
        const float     arrivalRadius   = (std::max)(halfCell, moveSpeed_ * Time::DeltaTime() * 1.5f);
        const float     arrivalRadiusSq = arrivalRadius * arrivalRadius;

        auto& path = pathFinder_.Path();
        while (!path.empty())
        {
            glm::vec3 toWaypoint = path.front() - selfPos;
            toWaypoint.y = 0.0f;
            if (toWaypoint.x * toWaypoint.x + toWaypoint.z * toWaypoint.z > arrivalRadiusSq)
                break;
            path.erase(path.begin());
        }

        // 全ウェイポイント到達 → パスを無効化して再探索待ち
        if (path.empty())
        {
            pathFinder_.ClearPath();
            return TickStatus::Running;
        }

        const glm::vec3 target   = path.front();
        glm::vec3       toTarget = target - selfPos;
        toTarget.y = 0.0f;

        const float horizDistSq = toTarget.x * toTarget.x + toTarget.z * toTarget.z;
        if (horizDistSq <= 1e-6f)
            return TickStatus::Failure;

        toTarget = glm::normalize(toTarget);
        glm::vec3 velocity = toTarget * moveSpeed_;
        velocity.y = Physics::GetLinearVelocity(context.EnemyCollider().BodyId()).y;
        Physics::SetLinearVelocity(context.EnemyCollider().BodyId(), velocity);

        // 移動方向に回転する
        auto& transform = context.EnemyTransform();
        glm::vec3 forward = transform.GetWorldRot() * glm::vec3(0, 0, -1);
        forward.y = 0.0f;
        if (glm::length2(forward) > 0.0001f)
        {
            forward = glm::normalize(forward);

            const float dot      = glm::clamp(glm::dot(forward, toTarget), -1.0f, 1.0f);
            const float angleRad = std::acos(dot);
            if (angleRad > glm::radians(rotateToleranceDeg_))
            {
                const glm::quat currentRot = transform.GetWorldRot();
                const glm::quat deltaRot = dot < -0.9999f
                    ? glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0))
                    : glm::rotation(forward, toTarget);

                const float maxStep = glm::radians(rotateSpeed_) * Time::DeltaTime();
                const float t       = glm::min(1.0f, maxStep / angleRad);
                transform.SetWorldRot(glm::slerp(currentRot, deltaRot * currentRot, t));
            }
        }

        if (animationNumber_ >= 0)
            context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animationNumber_);

        return TickStatus::Success;
    }

    void Action::ChasePlayerForPathFinding::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("heightGridMap_", heightGridMap_);
        ImGuiHelper::OnDrawInputField("moveSpeed_", moveSpeed_);                            
        ImGuiHelper::OnDrawInputField("rotateSpeed_", rotateSpeed_);
        ImGuiHelper::OnDrawInputField("rotateToleranceDeg_", rotateToleranceDeg_);
        ImGuiHelper::OnDrawInputField("maxPathCellRange_", maxPathCellRange_);
        ImGuiHelper::OnDrawInputField("maxClimbAngleDeg_", maxClimbAngleDeg_);
        ImGuiHelper::OnDrawInputField("searchIntervalSec_", searchIntervalSec_);
        ImGuiHelper::OnDrawInputField("animationNumber_", animationNumber_);
    }
}
