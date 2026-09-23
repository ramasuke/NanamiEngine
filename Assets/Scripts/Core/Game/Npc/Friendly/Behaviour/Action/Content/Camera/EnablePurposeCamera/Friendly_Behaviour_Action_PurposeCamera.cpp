#include "Friendly_Behaviour_Action_PurposeCamera.h"
#include "../../../TickContext/Friendly_Behaviour_TickContext.h"
#include "../../../../../../../PlayerAvatar/IPlayerAvatar.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::PurposeCamera::DoTick(const TickContext& context)
    {
        const auto player = context.Player();
        if (onPurposeCameraEnable_)
        {
            purposeCamera_->SetPriority(ENABLE_PURPOSE_CAMERA_PRIORITY);
            // カメラがNPCを映している間はプレイヤーを動かさない
            if (player)
                player->GetEventSceneStateMachine().OnDisable();
        }
        else
        {
            purposeCamera_->OnDisable();
            if (player)
                player->GetEventSceneStateMachine().OnEnable();
        }
        return TickStatus::Success;
    }

    void Action::PurposeCamera::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("purposeCamera_", purposeCamera_);
        ImGuiHelper::OnDrawInputField("onPurposeCameraEnable_", onPurposeCameraEnable_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::PurposeCamera)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::PurposeCamera)
#pragma endregion
