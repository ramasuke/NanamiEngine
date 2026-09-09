#include "../../Custom_RpcType.h"
#include "../../../../Game/PlayerAvatar/IPlayerAvatar.h"

namespace
{
    struct SyncAvatarStateRpcRegistration
    {
        SyncAvatarStateRpcRegistration()
        {
            GameCore::Network::SyncAvatarStateRpc::OnTargeted<GameCore::IPlayerAvatar>(
                [](GameCore::IPlayerAvatar& avatar, uint8_t stateValue) { avatar.ApplySyncState(stateValue); },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static SyncAvatarStateRpcRegistration s_syncAvatarStateRpcRegistration;
}
