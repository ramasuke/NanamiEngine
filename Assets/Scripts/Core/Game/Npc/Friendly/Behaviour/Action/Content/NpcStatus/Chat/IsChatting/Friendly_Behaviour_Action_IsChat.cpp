#include "Friendly_Behaviour_Action_IsChat.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickStatus IsChat::DoTick(const TickContext& context)
    {
        return context.IsChatting()
            ? TickStatus::Success
            : TickStatus::Failure;
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::IsChat)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::IsChat)
#pragma endregion
