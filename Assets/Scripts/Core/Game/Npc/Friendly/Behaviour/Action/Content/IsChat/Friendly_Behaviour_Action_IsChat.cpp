#include "Friendly_Behaviour_Action_IsChat.h"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    TickStatus IsChat::DoTick(const TickContext& context)
    {
        return context.IsChatting()
            ? TickStatus::Success
            : TickStatus::Failure;
    }
}
