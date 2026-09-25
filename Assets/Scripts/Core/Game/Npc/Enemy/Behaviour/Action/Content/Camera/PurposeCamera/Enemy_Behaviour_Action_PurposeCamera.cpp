#include "Enemy_Behaviour_Action_PurposeCamera.h"

#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::PurposeCamera::DoTick(const TickContext& context)
    {
        auto& purposeCamera = context.CatchPrefabObject<CineMachine::CineMachineVirtualCamera>(prefabPurposeCamera_);
        purposeCamera.SetPriority(priority_);

        // 権威側限定Tickなら、他ピアの同名子カメラも同じ優先度にする
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::PurposeCameraRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, prefabPurposeCamera_, priority_);
        }

        return TickStatus::Success;
    }

    void Action::PurposeCamera::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("prefabPurposeCamera_", prefabPurposeCamera_);
        ImGuiHelper::OnDrawInputField("priority_", priority_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PurposeCamera, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
