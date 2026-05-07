#include "Enemy_Behaviour_Action_ScenePurposeCamera.h"
#include "../../../../../../../../../../../Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ScenePurposeCamera::DoTick(const TickContext& context)
    {
        purposeCamera_->SetPriority(priority_);
        return TickStatus::Success;
    }

    void Action::ScenePurposeCamera::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("purposeCamera_", purposeCamera_);
        ImGuiHelper::OnDrawInputField("priority_", priority_);
    }
}
