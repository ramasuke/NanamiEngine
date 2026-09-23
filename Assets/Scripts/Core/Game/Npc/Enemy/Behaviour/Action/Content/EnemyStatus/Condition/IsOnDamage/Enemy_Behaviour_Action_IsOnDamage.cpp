#include "Enemy_Behaviour_Action_IsOnDamage.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::IsOnDamage::DoTick(const TickContext& context)
    {
        return context.IsOnDamage() ? TickStatus::Success : TickStatus::Failure;
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::IsOnDamage)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::IsOnDamage)
#pragma endregion
