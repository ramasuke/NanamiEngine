#include "Friendly_Behaviour_Action_NodeTickTimer.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::NodeTickTimer::DoTick(
        const TickContext& context)
    {
        during_secs_ += Time::DeltaTime();
        return during_secs_ > duration_secs_ ? TickStatus::Success : TickStatus::Failure;
    }
    
    void Action::NodeTickTimer::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("duration_secs_", duration_secs_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::NodeTickTimer)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::NodeTickTimer)
#pragma endregion
