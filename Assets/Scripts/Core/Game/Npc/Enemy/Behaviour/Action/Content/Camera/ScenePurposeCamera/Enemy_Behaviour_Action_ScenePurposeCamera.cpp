#include "Enemy_Behaviour_Action_ScenePurposeCamera.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ScenePurposeCamera::DoTick(const TickContext& context)
    {
        if (!purposeCamera_)
            return TickStatus::Failure;

        purposeCamera_->SetPriority(priority_);

        // 権威側限定Tickなら、他ピアのシーン上の同じカメラも同じ優先度にする
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::ScenePurposeCameraRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, purposeCamera_->GetGuid(), priority_);
        }
        return TickStatus::Success;
    }

    void Action::ScenePurposeCamera::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("purposeCamera_", purposeCamera_);
        ImGuiHelper::OnDrawInputField("priority_", priority_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ScenePurposeCamera)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ScenePurposeCamera)
#pragma endregion
