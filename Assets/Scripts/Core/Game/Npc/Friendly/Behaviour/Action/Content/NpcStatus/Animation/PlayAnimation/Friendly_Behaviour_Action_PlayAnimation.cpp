#include "Friendly_Behaviour_Action_PlayAnimation.h"

#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::PlayAnimation)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::PlayAnimation)
#pragma endregion
