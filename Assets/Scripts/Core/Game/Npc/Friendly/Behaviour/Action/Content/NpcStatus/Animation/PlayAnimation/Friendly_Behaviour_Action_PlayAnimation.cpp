#include "Friendly_Behaviour_Action_PlayAnimation.h"

#include "../../../../../../../../../../../../Engine/Module/Component/Animator/Animator.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::PlayAnimation::DoTick(const TickContext& context)
    {
        context.NpcAnimator().Param<int>(ANIMATOR_PARAM_NAME).Set(animatorSetParamNumber_);
        
        return TickStatus::Success;
    }

    void Action::PlayAnimation::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("animatorSetParamNumber", animatorSetParamNumber_);
    }
}
