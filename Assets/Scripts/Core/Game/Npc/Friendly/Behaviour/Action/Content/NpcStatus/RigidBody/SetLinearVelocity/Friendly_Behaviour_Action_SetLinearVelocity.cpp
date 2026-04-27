#include "Friendly_Behaviour_Action_SetLinearVelocity.h"

#include "../../../../../../../../../../../../Engine/Module/Component/Collider/ICollider.h"
#include "../../../../../../../../../../../../Engine/Module/Physics/Physics_.h"

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
