#include "Friendly_Behaviour_Action_SetLinearVelocity.h"

#include "../../../../../../../../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ICollider.h"
#include "../../../../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::SetLinearVelocity::DoTick(
        const TickContext& context)
    {
        Physics::SetLinearVelocity(context.NpcCollider().BodyId(), setVelocity_);
        return TickStatus::Success;
    }

    void Action::SetLinearVelocity::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("setVelocity_", setVelocity_);
    }
}
