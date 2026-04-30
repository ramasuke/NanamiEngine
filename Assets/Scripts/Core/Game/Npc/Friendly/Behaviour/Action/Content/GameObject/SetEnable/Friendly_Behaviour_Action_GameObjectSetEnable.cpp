#include "Friendly_Behaviour_Action_GameObjectSetEnable.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::GameObjectSetEnable::DoTick(
        const TickContext& context)
    {
        enableGameObject_->SetEnable(isEnable_);
        return TickStatus::Success;
    }

    void Action::GameObjectSetEnable::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("enableGameObject_", enableGameObject_);
        ImGuiHelper::OnDrawInputField("isEnable_", isEnable_);
    }
}
