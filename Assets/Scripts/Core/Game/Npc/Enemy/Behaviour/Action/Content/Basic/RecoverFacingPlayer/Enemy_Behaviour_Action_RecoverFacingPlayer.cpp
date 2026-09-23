#include "Enemy_Behaviour_Action_RecoverFacingPlayer.h"

#include <algorithm>
#include <cmath>

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
        // その場旋回アニメをやめる角度(faceToleranceDeg_ がこれより大きければそちら)
        constexpr float TURN_IN_PLACE_END_DEG = 2.0f;
    }

    TickStatus Action::RecoverFacingPlayer::DoTick(const TickContext& context)
    {
        const float now = Time::CurrentTime();
        if (isRunning_ && now - lastTickTime_ > INTERRUPT_GAP_SECS)
            isRunning_ = false;
        lastTickTime_ = now;

        if (!isRunning_)
        {
            const float lo = (std::min)(minSeconds_, maxSeconds_);
            const float hi = (std::max)(minSeconds_, maxSeconds_);
            duration_secs_ = std::uniform_real_distribution<float>(lo, hi)(rng_);
            during_secs_   = 0.0f;
            isWalkTurning_   = false;
            turnInPlaceSign_ = 0;
            isRunning_       = true;
        }

        auto& transform = context.EnemyTransform();
        auto& rigidBody = context.EnemyRigidBody();
        const bool isHolding = during_secs_ < holdSeconds_;

        float angle      = 0.0f;
        bool  hasAngle   = false;
        glm::vec3 toPlayer(0.0f);
        glm::vec3 playerPos;
        if (!isHolding && context.NearestPlayerPosition(transform.GetWorldPos(), playerPos))
        {
            toPlayer = playerPos - transform.GetWorldPos();
            hasAngle = SignedHorizontalAngleDeg(transform, toPlayer, angle);
        }
        const float absAngle = std::abs(angle);

        // 一度歩き出したら向き終わるまで歩き続ける(しきい値付近でアニメが切り替わり続けないように)
        const bool canWalkTurn = turnMoveSpeed_ > 0.0f && turnAnimationNumber_ >= 0;
        if (hasAngle && canWalkTurn && absAngle > turnWalkAngle_)
            isWalkTurning_ = true;
        if (!hasAngle || absAngle <= faceToleranceDeg_)
            isWalkTurning_ = false;

        glm::vec3 velocity(0.0f);
        if (hasAngle)
        {
            if (isWalkTurning_)
            {
                glm::vec3 forward = transform.GetWorldRot() * glm::vec3(0, 0, -1);
                forward.y = 0.0f;
                if (glm::length2(forward) > 1e-6f)
                    velocity = glm::normalize(forward) * turnMoveSpeed_;
                RotateTowardsHorizontal(transform, toPlayer, turnRotateSpeed_);
            }
            else
            {
                RotateTowardsHorizontal(transform, toPlayer, rotateSpeed_);
            }
        }
        velocity.y = rigidBody.LinearVelocity().y;
        rigidBody.SetLinearVelocity(velocity);

        // その場旋回も一度始めたらほぼ向き終わるまで同じ側を維持する
        if (!hasAngle || isWalkTurning_ || absAngle <= (std::max)(faceToleranceDeg_, TURN_IN_PLACE_END_DEG))
            turnInPlaceSign_ = 0;
        else if (turnInPlaceSign_ == 0 && absAngle > turnInPlaceAngle_)
            turnInPlaceSign_ = angle > 0.0f ? 1 : -1;

        int animation = isWalkTurning_ ? turnAnimationNumber_ : animationNumber_;
        const int turnInPlaceAnimation = turnInPlaceSign_ > 0 ? turnRightAnimationNumber_
                                       : turnInPlaceSign_ < 0 ? turnLeftAnimationNumber_ : -1;
        if (turnInPlaceAnimation >= 0)
            animation = turnInPlaceAnimation;
        if (animation >= 0)
            context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animation);

        during_secs_ += Time::DeltaTime();
        const bool isFacing = hasAngle && faceToleranceDeg_ > 0.0f && absAngle <= faceToleranceDeg_;
        if (!isFacing && during_secs_ < duration_secs_)
            return TickStatus::Running;

        isRunning_ = false;
        return TickStatus::Success;
    }

    void Action::RecoverFacingPlayer::DoReset()
    {
        isRunning_ = false;
    }

    void Action::RecoverFacingPlayer::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("minSeconds_", minSeconds_);
        ImGuiHelper::OnDrawInputField("maxSeconds_", maxSeconds_);
        ImGuiHelper::OnDrawInputField("rotateSpeed_", rotateSpeed_);
        ImGuiHelper::OnDrawInputField("animationNumber_", animationNumber_);
        ImGuiHelper::OnDrawInputField("holdSeconds_", holdSeconds_);
        ImGuiHelper::OnDrawInputField("turnRotateSpeed_", turnRotateSpeed_);
        ImGuiHelper::OnDrawInputField("turnWalkAngle_", turnWalkAngle_);
        ImGuiHelper::OnDrawInputField("turnMoveSpeed_", turnMoveSpeed_);
        ImGuiHelper::OnDrawInputField("turnAnimationNumber_", turnAnimationNumber_);
        ImGuiHelper::OnDrawInputField("faceToleranceDeg_", faceToleranceDeg_);
        ImGuiHelper::OnDrawInputField("turnLeftAnimationNumber_", turnLeftAnimationNumber_);
        ImGuiHelper::OnDrawInputField("turnRightAnimationNumber_", turnRightAnimationNumber_);
        ImGuiHelper::OnDrawInputField("turnInPlaceAngle_", turnInPlaceAngle_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::RecoverFacingPlayer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::RecoverFacingPlayer);
#pragma endregion
