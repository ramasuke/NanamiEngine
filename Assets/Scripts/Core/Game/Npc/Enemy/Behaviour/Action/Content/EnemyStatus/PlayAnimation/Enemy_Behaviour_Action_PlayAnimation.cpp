#include "Enemy_Behaviour_Action_PlayAnimation.h"

#include "../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"
#include "../../../../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PlayAnimation::DoTick(const TickContext& context)
    {
        context.EnemyAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animatorSetParamNumber_);
        
        if (waitAnimationSound_secs_.Tick(context) == TickStatus::Success)
        {
            animationSound_.Tick(context);
            waitAnimationSound_secs_.Reset();
        }
        return TickStatus::Success;
    }

    void Action::PlayAnimation::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("animatorSetParamNumber", animatorSetParamNumber_);
        ImGuiHelper::OnDrawInputField("waitAnimationSound_secs_", waitAnimationSound_secs_);
        ImGuiHelper::OnDrawInputField("animationSound_", animationSound_);
    }
}
