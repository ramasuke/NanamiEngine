#include "Enemy_Behaviour_Action_CircleAroundPlayer.h"

#include <algorithm>

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../Common/Enemy_Behaviour_FaceDirection.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    namespace
    {
        // 前回の Tick からこれ以上空いたら、途中で打ち切られたとみなして新しく始める
        constexpr float INTERRUPT_GAP_SECS = 0.2f;
        // 実際の移動量が期待値のこの割合を下回った状態が STUCK_SECS 続いたら詰まりとみなす
        constexpr float STUCK_PROGRESS_RATE = 0.2f;
        constexpr float STUCK_SECS          = 0.3f;
    }

    TickStatus Action::CircleAroundPlayer::DoTick(const TickContext& context)
    {
        auto& transform = context.EnemyTransform();
        const glm::vec3 selfPos = transform.GetWorldPos();

        glm::vec3 playerPos;
        if (!context.NearestPlayerPosition(selfPos, playerPos))
            return TickStatus::Failure;

        const float now = Time::CurrentTime();
        if (isRunning_ && now - lastTickTime_ > INTERRUPT_GAP_SECS)
            isRunning_ = false;
        lastTickTime_ = now;

        const float delta = Time::DeltaTime();

        if (!isRunning_)
        {
            const float lo = (std::min)(minSeconds_, maxSeconds_);
            const float hi = (std::max)(minSeconds_, maxSeconds_);
            duration_secs_ = std::uniform_real_distribution<float>(lo, hi)(rng_);
            direction_     = std::bernoulli_distribution(0.5)(rng_) ? 1.0f : -1.0f;
            during_secs_   = 0.0f;
            stuck_secs_    = 0.0f;
            hasFlipped_    = false;
            isRunning_     = true;
        }
        else if (delta > 0.0f)
        {
            // 前フレームに指示した速度に対して実際どれだけ進めたか
            glm::vec3 moved = selfPos - lastPosition_;
            moved.y = 0.0f;
            const float expected = moveSpeed_ * delta;
            if (expected > 0.0f && glm::length(moved) < expected * STUCK_PROGRESS_RATE)
                stuck_secs_ += delta;
            else
                stuck_secs_ = 0.0f;

            if (stuck_secs_ >= STUCK_SECS)
            {
                if (hasFlipped_)
                {
                    isRunning_ = false;
                    return TickStatus::Success;
                }
                direction_  = -direction_;
                hasFlipped_ = true;
                stuck_secs_ = 0.0f;
            }
        }
        lastPosition_ = selfPos;

        glm::vec3 toPlayer = playerPos - selfPos;
        toPlayer.y = 0.0f;
        const float distance = glm::length(toPlayer);
        if (distance < 1e-3f)
        {
            isRunning_ = false;
            return TickStatus::Failure;
        }
        const glm::vec3 toPlayerDir = toPlayer / distance;

        float radius = desiredRadius_;
        if (radiusShrinkPerSec_ > 0.0f)
        {
            if (distance <= minRadius_)
            {
                isRunning_ = false;
                return TickStatus::Success;
            }
            radius = (std::max)(desiredRadius_ - radiusShrinkPerSec_ * during_secs_, minRadius_);
        }

        // 接線方向 + 半径を保つための補正
        const glm::vec3 tangent = glm::cross(glm::vec3(0, 1, 0), toPlayerDir) * direction_;
        glm::vec3 moveDir = tangent + toPlayerDir * ((distance - radius) * radiusGain_ / (std::max)(radius, 1e-3f));
        moveDir.y = 0.0f;
        if (glm::length2(moveDir) < 1e-6f)
            moveDir = tangent;
        moveDir = glm::normalize(moveDir);

        auto& rigidBody = context.EnemyRigidBody();
        glm::vec3 velocity = moveDir * moveSpeed_;
        velocity.y = rigidBody.LinearVelocity().y;
        rigidBody.SetLinearVelocity(velocity);

        RotateTowardsHorizontal(transform, moveDir, rotateSpeed_);

        if (animationNumber_ >= 0)
            context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animationNumber_);

        during_secs_ += delta;
        if (during_secs_ < duration_secs_)
            return TickStatus::Running;

        isRunning_ = false;
        return TickStatus::Success;
    }

    void Action::CircleAroundPlayer::DoReset()
    {
        isRunning_ = false;
    }

    void Action::CircleAroundPlayer::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("moveSpeed_", moveSpeed_);
        ImGuiHelper::OnDrawInputField("rotateSpeed_", rotateSpeed_);
        ImGuiHelper::OnDrawInputField("minSeconds_", minSeconds_);
        ImGuiHelper::OnDrawInputField("maxSeconds_", maxSeconds_);
        ImGuiHelper::OnDrawInputField("desiredRadius_", desiredRadius_);
        ImGuiHelper::OnDrawInputField("radiusGain_", radiusGain_);
        ImGuiHelper::OnDrawInputField("animationNumber_", animationNumber_);
        ImGuiHelper::OnDrawInputField("radiusShrinkPerSec_", radiusShrinkPerSec_);
        ImGuiHelper::OnDrawInputField("minRadius_", minRadius_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::CircleAroundPlayer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::CircleAroundPlayer);
#pragma endregion
