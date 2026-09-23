#include "Friendly_Behaviour_Action_SetFreezePhysics.h"

#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::SetFreezePhysics::DoTick(
        const TickContext& context)
    {
        context.NpcRigidBody().SetFreezePhysics(constraints_);
        return TickStatus::Success;
    }

    void Action::SetFreezePhysics::DoDrawGui()
    {
        Physics::DrawConstraintCheckBoxsGui(constraints_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::SetFreezePhysics)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::SetFreezePhysics)
#pragma endregion
