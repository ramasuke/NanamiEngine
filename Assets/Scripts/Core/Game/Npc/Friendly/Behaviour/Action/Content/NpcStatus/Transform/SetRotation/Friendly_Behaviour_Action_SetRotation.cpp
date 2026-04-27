#include "Friendly_Behaviour_Action_SetRotation.h"

#include "../../../../../../../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../../../../../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::SetRotation::DoTick(const TickContext& context)
    {
        context.NpcTransform().LookAtY(lookAtDirection_);
        
        return TickStatus::Success;
    }

    void Action::SetRotation::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("lookAtDirection", lookAtDirection_);
    }
}
