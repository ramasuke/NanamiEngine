#include "Friendly_Behaviour_Action_PurposeCamera.h"
#include "../../../TickContext/Friendly_Behaviour_TickContext.h"
#include "../../../../../../../PlayerAvatar/ControlLock/PlayerAvatar_ControlLock.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    namespace
    {
        constexpr const char* PURPOSE_CAMERA_CONTROL_LOCK_TAG = "NpcPurposeCamera";
    }

    TickStatus Action::PurposeCamera::DoTick(const TickContext& context)
    {
        if (onPurposeCameraEnable_)
        {
            purposeCamera_->SetPriority(ENABLE_PURPOSE_CAMERA_PRIORITY);
            // NOTE: カメラが NPC を映している間はプレイヤーを動かさない
            PlayerAvatar::LockControlBy(context.NpcGameObject(), PURPOSE_CAMERA_CONTROL_LOCK_TAG);
        }
        else
        {
            purposeCamera_->OnDisable();
            PlayerAvatar::UnlockControlBy(context.NpcGameObject(), PURPOSE_CAMERA_CONTROL_LOCK_TAG);
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
NANAMI_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::PurposeCamera, GameCore::Npc::Friendly::Behaviour::ActionBase);
#pragma endregion
