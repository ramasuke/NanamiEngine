#include "Enemy_Behaviour_Action_PlayAnimation.h"

#include "Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PlayAnimation::DoTick(const TickContext& context)
    {
        auto& param = context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME);
        // NOTE: Sequence は後ろの Wait が終わるまで毎フレームこのノードを Tick し直すので、音はアニメーションに入ったときに 1 回だけ鳴らす
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
        return TickStatus::Success;
    }

    void Action::PlayAnimation::DoReset()
    {
        waitAnimationSound_secs_.Reset();
        isSoundPending_ = true;
    }

    void Action::PlayAnimation::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("animatorSetParamNumber", animatorSetParamNumber_);
        ImGuiHelper::OnDrawInputField("waitAnimationSound_secs_", waitAnimationSound_secs_);
        ImGuiHelper::OnDrawInputField("animationSound_", animationSound_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PlayAnimation, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
