#include "Friendly_Behaviour_Action_SetLinearVelocity.h"

#include "../../../../../../../../../../../../Engine/Module/Physics/Component/RigidBody/Engine_Physics_RigidBody.h"

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
