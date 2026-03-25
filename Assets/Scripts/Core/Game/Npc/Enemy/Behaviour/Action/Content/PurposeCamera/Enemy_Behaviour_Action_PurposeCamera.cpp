#include "Enemy_Behaviour_Action_PurposeCamera.h"

#include "../../../../../../../../../../Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PurposeCamera::DoTick(const TickContext& context)
    {
        auto& purposeCamera = context.CatchPrefabObject<CineMachine::CineMachineVirtualCamera>(prefabPurposeCamera_);
        purposeCamera.SetPriority(priority_);
        
        return TickStatus::Success;
    }

    void Action::PurposeCamera::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("prefabPurposeCamera_", prefabPurposeCamera_);
        ImGuiHelper::OnDrawInputField("priority_", priority_);
    }
}
