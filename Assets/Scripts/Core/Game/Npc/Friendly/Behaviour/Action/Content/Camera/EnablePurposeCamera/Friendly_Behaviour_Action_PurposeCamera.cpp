#include "Friendly_Behaviour_Action_PurposeCamera.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::PurposeCamera::DoTick(const TickContext& context)
    {
        if (onPurposeCameraEnable_)
        {
            purposeCamera_->SetPriority(ENABLE_PURPOSE_CAMERA_PRIORITY);
        }
        else
        {
            purposeCamera_->OnDisable();
        }
        return TickStatus::Success;
    }

    void Action::PurposeCamera::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("purposeCamera_", purposeCamera_);
        ImGuiHelper::OnDrawInputField("onPurposeCameraEnable_", onPurposeCameraEnable_);
    }
}
