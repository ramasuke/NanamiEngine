#include "Enemy_Behaviour_Action_Attack.h"

#include "../../../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"
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
            GamePlay::Sound::SoundPlayer::PlaySe(*attackSound_.get(), context.EnemyTransform().GetWorldPos());
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
