#include "Friendly_Behaviour_Action_SetLinearVelocity.h"

#include "Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::SetLinearVelocity::DoTick(
        const TickContext& context)
    {
        context.NpcRigidBody().SetLinearVelocity(setVelocity_);
        return TickStatus::Success;
    }

    void Action::SetLinearVelocity::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("setVelocity_", setVelocity_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::SetLinearVelocity, GameCore::Npc::Friendly::Behaviour::ActionBase);
#pragma endregion
