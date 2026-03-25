#include "Enemy_Behaviour_Action_IsOnDamage.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::IsOnDamage::DoTick(const TickContext& context)
    {
        return context.IsOnDamage() ? TickStatus::Success : TickStatus::Failure;
    }
}
