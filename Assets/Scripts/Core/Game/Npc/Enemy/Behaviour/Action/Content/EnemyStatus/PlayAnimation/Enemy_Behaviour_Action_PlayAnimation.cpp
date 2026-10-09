#include "Enemy_Behaviour_Action_PlayAnimation.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PlayAnimation::DoTick(const TickContext& context)
    {
        auto& param = context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME);
        // NOTE: 親ノードから毎フレーム Tick し直されるので、音はアニメーションに入ったときだけ鳴らす
        if (param.Get() != animatorSetParamNumber_)
        {
            waitAnimationSound_secs_.Reset();
            isSoundPending_ = true;
        }
        param.Set(animatorSetParamNumber_);

        if (isSoundPending_ && waitAnimationSound_secs_.Tick(context) == TickStatus::Success)
        {
            animationSound_.Tick(context);
            isSoundPending_ = false;
        }

        if (holdSeconds_ <= 0.0f)
            return TickStatus::Success;

        // NOTE: 前回の Tick で呼ばれなかった = 入り直したので待ち直す
        if (lastTickIndex_ + 1 != context.TickIndex())
            hold_secs_ = 0.0f;
        lastTickIndex_ = context.TickIndex();

        const bool done = holdSeconds_ <= hold_secs_;
        hold_secs_ += Time::DeltaTime();
        return done ? TickStatus::Success : TickStatus::Running;
    }

    void Action::PlayAnimation::DoReset()
    {
        waitAnimationSound_secs_.Reset();
        isSoundPending_ = true;
        hold_secs_ = 0.0f;
    }

    void Action::PlayAnimation::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("animatorSetParamNumber", animatorSetParamNumber_);
        ImGuiHelper::OnDrawInputField("holdSeconds_", holdSeconds_);
        ImGuiHelper::OnDrawInputField("waitAnimationSound_secs_", waitAnimationSound_secs_);
        ImGuiHelper::OnDrawInputField("animationSound_", animationSound_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PlayAnimation, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
