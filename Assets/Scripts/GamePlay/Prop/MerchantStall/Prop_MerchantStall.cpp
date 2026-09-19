#include "Prop_MerchantStall.h"

namespace GamePlay::Prop
{
    void MerchantStall::FocusCamera() const
    {
        if (const auto camera = shopCamera_.get())
            camera->SetPriority(focusPriority_);
    }

    void MerchantStall::RestoreCamera() const
    {
        if (const auto camera = shopCamera_.get())
            camera->SetPriority(CineMachine::DISABLE_PRIORITY);
    }

    void MerchantStall::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("shopCamera_", shopCamera_);
        ImGuiHelper::OnDrawInputField("focusPriority_", focusPriority_);
    }
}
