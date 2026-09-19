#include "../../Custom_RpcType.h"
#include "../../../../Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"

namespace
{
    struct WakeUpPlayerRpcRegistration
    {
        WakeUpPlayerRpcRegistration()
        {
            GameCore::Network::WakeUpPlayerRpc::OnTargeted<GameCore::IPlayerAvatar>(
                [](GameCore::IPlayerAvatar& avatar)
                {
                    if (!avatar.PlayerStatus().IsDowned())
                        return;
                    avatar.PlayerStatus().Revive();
                    avatar.GetEventSceneStateMachine().OnChangeState(
                        GameCore::PlayerAvatar::EventSceneStateType::GetUp);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::OnlyIfOwner);
        }
    };
    static WakeUpPlayerRpcRegistration s_wakeUpPlayerRpcRegistration;
}
