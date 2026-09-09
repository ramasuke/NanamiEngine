#include "Enemy_Behaviour_Action_ShakeCamera.h"

#include "../../../../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ShakeCamera::DoTick(const TickContext& context)
    {
        CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(intensity_, duration_);

        // 権威側限定Tickなら、他ピア自身のカメラも同じ強さ/長さで揺らす
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::ShakeCameraRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, intensity_, duration_);
        }

        return TickStatus::Success;
    }

    void Action::ShakeCamera::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("intensity_", intensity_);
        ImGuiHelper::OnDrawInputField("duration_", duration_);
    }
}
