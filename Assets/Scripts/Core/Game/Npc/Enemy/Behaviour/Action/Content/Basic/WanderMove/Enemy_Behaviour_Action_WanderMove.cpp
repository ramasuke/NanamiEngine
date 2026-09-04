#include "Enemy_Behaviour_Action_WanderMove.h"

#include <algorithm>
#include <cmath>
#include <random>

#include "../../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "../../../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../glm/gtx/quaternion.hpp"
#include "../glm/gtx/vector_angle.hpp"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::WanderMove::DoTick(const TickContext& context)
    {
        const auto grid = heightGridMap_.get();
        if (!grid)
            return TickStatus::Failure;

        const glm::vec3 selfPos = context.EnemyTransform().GetWorldPos();

        // 初回のみスポーン地点を記録し、即座に移動開始させる
        if (!spawnPosInitialized_)
        {
            spawnPos_            = selfPos;
            spawnPosInitialized_ = true;
            currentWaitTime_     = 0.0f;
        }

        // Idle: 待機してからランダム目標地点を決定して Moving へ
        if (state_ == State::Idle)
        {
            if (animationIdleNumber_ >= 0)
                context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animationIdleNumber_);

            waitTimer_ += Time::DeltaTime();
            if (waitTimer_ < currentWaitTime_)
                return TickStatus::Running;

            // スポーン地点を中心に wanderRadius_ 内でランダムな目標地点を生成
            std::uniform_real_distribution<float> angleDist(0.0f, glm::pi<float>() * 2.0f);
            std::uniform_real_distribution<float> radiusDist(0.0f, wanderRadius_);
            const float angle  = angleDist(rng_);
            const float radius = radiusDist(rng_);
            wanderTarget_ = spawnPos_ + glm::vec3(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);

            pathFinder_.ClearPath();
            state_ = State::Moving;
            return TickStatus::Running;
        }

        // Moving: PathFinding で wanderTarget_ へ移動する
        pathFinder_.Tick(grid, selfPos, { wanderTarget_ },
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

        // 全ウェイポイント到達 → Idle に戻りランダム待機時間を設定
        if (path.empty())
        {
            pathFinder_.ClearPath();
            state_     = State::Idle;
            waitTimer_ = 0.0f;
            std::uniform_real_distribution<float> waitDist(waitTimeMin_, waitTimeMax_);
            currentWaitTime_ = waitDist(rng_);
            return TickStatus::Running;
        }

        const glm::vec3 target   = path.front();
        glm::vec3       toTarget = target - selfPos;
        toTarget.y = 0.0f;

        const float horizDistSq = toTarget.x * toTarget.x + toTarget.z * toTarget.z;
        if (horizDistSq <= 1e-6f)
            return TickStatus::Running;

        toTarget = glm::normalize(toTarget);
        glm::vec3 velocity = toTarget * moveSpeed_;
        velocity.y = Physics::GetLinearVelocity(context.EnemyCollider().BodyId()).y;
        Physics::SetLinearVelocity(context.EnemyCollider().BodyId(), velocity);

        // 移動方向に回転する
        auto&     transform = context.EnemyTransform();
        glm::vec3 forward   = transform.GetWorldRot() * glm::vec3(0, 0, -1);
        forward.y = 0.0f;
        if (glm::length2(forward) > 0.0001f)
        {
            forward = glm::normalize(forward);
            const float dot      = glm::clamp(glm::dot(forward, toTarget), -1.0f, 1.0f);
            const float angleRad = std::acos(dot);
            if (angleRad > glm::radians(rotateToleranceDeg_))
            {
                const glm::quat currentRot = transform.GetWorldRot();
                const glm::quat deltaRot   = dot < -0.9999f
                    ? glm::angleAxis(glm::pi<float>(), glm::vec3(0, 1, 0))
                    : glm::rotation(forward, toTarget);
                const float maxStep = glm::radians(rotateSpeed_) * Time::DeltaTime();
                const float t       = glm::min(1.0f, maxStep / angleRad);
                transform.SetWorldRot(glm::slerp(currentRot, deltaRot * currentRot, t));
            }
        }

        if (animationMoveNumber_ >= 0)
            context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animationMoveNumber_);

        return TickStatus::Success;
    }

    void Action::WanderMove::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("heightGridMap_"      , heightGridMap_);
        ImGuiHelper::OnDrawInputField("wanderRadius_"       , wanderRadius_);
        ImGuiHelper::OnDrawInputField("waitTimeMin_"        , waitTimeMin_);
        ImGuiHelper::OnDrawInputField("waitTimeMax_"        , waitTimeMax_);
        ImGuiHelper::OnDrawInputField("moveSpeed_"          , moveSpeed_);
        ImGuiHelper::OnDrawInputField("rotateSpeed_"        , rotateSpeed_);
        ImGuiHelper::OnDrawInputField("rotateToleranceDeg_" , rotateToleranceDeg_);
        ImGuiHelper::OnDrawInputField("maxPathCellRange_"   , maxPathCellRange_);
        ImGuiHelper::OnDrawInputField("maxClimbAngleDeg_"   , maxClimbAngleDeg_);
        ImGuiHelper::OnDrawInputField("searchIntervalSec_"  , searchIntervalSec_);
        ImGuiHelper::OnDrawInputField("animationMoveNumber_", animationMoveNumber_);
        ImGuiHelper::OnDrawInputField("animationIdleNumber_", animationIdleNumber_);
    }
}
