#include "Friendly_Behaviour_Action_GameObjectSetEnable.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::GameObjectSetEnable)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Friendly::Behaviour::ActionBase,
    GameCore::Npc::Friendly::Behaviour::Action::GameObjectSetEnable)
#pragma endregion
