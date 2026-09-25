#include "Enemy_Behaviour_Action_Attack.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "../../Basic/Common/Enemy_Behaviour_FaceDirection.h"
#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "../../../../../AttackArea/Enemy_AttackArea.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PhysicsAttack::DoTick(const TickContext& context)
    {
        context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animationNumber_);
        const float delta = Time::DeltaTime();
        during_secs_ += delta;

        UpdateMovement(context);

        // 発生タイミングで一度攻撃
        if (!isAttacked_ && during_secs_ >= normalAttackOccurrenceDuration_secs_)
        {
            auto& attackArea = context.CatchPrefabObject<AttackArea>(attackAreaName_);
            attackArea.PhysicsAttack(context.EnemyGameObject(), attackPower_);

            const glm::vec3 position = context.EnemyTransform().GetWorldPos();
            if (attackSound_)
                GamePlay::Sound::SoundPlayer::PlaySe(*attackSound_.get(), position);

            // 権威側限定Tickなら、他ピアの同じ AttackArea も発火させる(被弾判定は各ピアが自分の所有アバターに対して行う)。
            // 攻撃音も同様に鳴らさせる
            if (context.IsNetworkAuthority())
            {
                GameCore::Network::AttackAreaFireRpc::Send(
                    attackArea.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, attackPower_);

                if (attackSound_)
                {
                    GameCore::Network::PlaySeRpc::Send(
                        context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, attackSound_->GetGuid(), position);
                }
            }
            isAttacked_ = true;
        }

        if (during_secs_ >= normalAttackDuration_secs_)
        {
            during_secs_ = 0.0f;
            isAttacked_  = false;
            finishedAttackWriteBlackBoard_.Tick(context);
            return TickStatus::Success;
        }

        return TickStatus::Running;
    }

    void Action::PhysicsAttack::UpdateMovement(const TickContext& context) const
    {
        const bool isTracking = trackRotateSpeed_ > 0.0f && during_secs_ <= trackEnd_secs_;
        if (!isTracking && lungeSpeed_ <= 0.0f)
            return;

        auto& transform = context.EnemyTransform();
        const glm::vec3 selfPos = transform.GetWorldPos();
        glm::vec3 playerPos;
        const bool hasPlayer = context.NearestPlayerPosition(selfPos, playerPos);

        if (isTracking && hasPlayer)
            RotateTowardsHorizontal(transform, playerPos - selfPos, trackRotateSpeed_);

        if (lungeSpeed_ <= 0.0f)
            return;

        bool isLunging = during_secs_ >= lungeStart_secs_ && during_secs_ <= lungeEnd_secs_;
        if (isLunging && hasPlayer && lungeStopDistance_ > 0.0f)
        {
            glm::vec3 toPlayer = playerPos - selfPos;
            toPlayer.y = 0.0f;
            isLunging = glm::length(toPlayer) > lungeStopDistance_;
        }

        auto& rigidBody = context.EnemyRigidBody();
        glm::vec3 velocity(0.0f);
        if (isLunging)
        {
            glm::vec3 forward = transform.GetWorldRot() * glm::vec3(0, 0, -1);
            forward.y = 0.0f;
            if (glm::length2(forward) > 1e-6f)
                velocity = glm::normalize(forward) * lungeSpeed_;
        }
        velocity.y = rigidBody.LinearVelocity().y;
        rigidBody.SetLinearVelocity(velocity);
    }

    void Action::PhysicsAttack::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("attackAreaName_", attackAreaName_);
        ImGuiHelper::OnDrawInputField("attackPower_", attackPower_);
        ImGuiHelper::OnDrawInputField("normalAttackOccurrenceDuration_secs_", normalAttackOccurrenceDuration_secs_);
        ImGuiHelper::OnDrawInputField("normalAttackDuration_secs_", normalAttackDuration_secs_);
        ImGuiHelper::OnDrawInputField("animationNumber_", animationNumber_);
        ImGuiHelper::OnDrawInputField("finishedAttackWriteBlackBoard_", finishedAttackWriteBlackBoard_);
        ImGuiHelper::OnDrawInputField("attackSound_", attackSound_);
        ImGuiHelper::OnDrawInputField("trackRotateSpeed_", trackRotateSpeed_);
        ImGuiHelper::OnDrawInputField("trackEnd_secs_", trackEnd_secs_);
        ImGuiHelper::OnDrawInputField("lungeSpeed_", lungeSpeed_);
        ImGuiHelper::OnDrawInputField("lungeStart_secs_", lungeStart_secs_);
        ImGuiHelper::OnDrawInputField("lungeEnd_secs_", lungeEnd_secs_);
        ImGuiHelper::OnDrawInputField("lungeStopDistance_", lungeStopDistance_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PhysicsAttack, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
