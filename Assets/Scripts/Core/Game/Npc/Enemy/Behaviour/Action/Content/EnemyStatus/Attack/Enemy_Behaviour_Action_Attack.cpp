#include "Enemy_Behaviour_Action_Attack.h"

#include "../../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "../../../../../AttackArea/Enemy_AttackArea.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PhysicsAttack::DoTick(const TickContext& context)
    {
        context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animationNumber_);
        const float delta = Time::DeltaTime();
        during_secs_ += delta;

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

    void Action::PhysicsAttack::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("attackAreaName_", attackAreaName_);
        ImGuiHelper::OnDrawInputField("attackPower_", attackPower_);
        ImGuiHelper::OnDrawInputField("normalAttackOccurrenceDuration_secs_", normalAttackOccurrenceDuration_secs_);
        ImGuiHelper::OnDrawInputField("normalAttackDuration_secs_", normalAttackDuration_secs_);
        ImGuiHelper::OnDrawInputField("animationNumber_", animationNumber_);
        ImGuiHelper::OnDrawInputField("finishedAttackWriteBlackBoard_", finishedAttackWriteBlackBoard_);
        ImGuiHelper::OnDrawInputField("attackSound_", attackSound_);
    }
}
