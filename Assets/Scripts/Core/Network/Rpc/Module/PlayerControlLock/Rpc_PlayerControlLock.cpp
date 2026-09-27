#include "Rpc_PlayerControlLock.h"

#include <memory>

#include "../../Custom_RpcType.h"
#include "../../../../Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../../Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"

namespace
{
    std::weak_ptr<GameCore::IPlayerAvatar> s_lockedAvatar;
}

void GameCore::Network::ApplyPlayerControlLock(const bool isLock)
{
    if (isLock)
    {
        const auto avatar = PlayerAvatar::Owner();
        if (!avatar || avatar->PlayerStatus().IsDowned())
            return;
        
        avatar->GetEventSceneStateMachine().OnDisable();
        s_lockedAvatar = avatar;
        return;
    }

    const auto avatar = s_lockedAvatar.lock();
    s_lockedAvatar.reset();
    if (!avatar)
        return;
    
    avatar->GetEventSceneStateMachine().OnEnable();
    avatar->GetEventSceneStateMachine().OnChangeState(PlayerAvatar::EventSceneStateType::Idle);
}

namespace
{
    struct PlayerControlLockRpcRegistration
    {
        PlayerControlLockRpcRegistration()
        {
            GameCore::Network::PlayerControlLockRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, const bool isLock)
                {
                    GameCore::Network::ApplyPlayerControlLock(isLock);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static PlayerControlLockRpcRegistration s_playerControlLockRpcRegistration;
}
