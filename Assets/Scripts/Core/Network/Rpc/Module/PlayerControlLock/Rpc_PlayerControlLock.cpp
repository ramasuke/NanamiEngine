#include "Rpc_PlayerControlLock.h"

#include "../../Custom_RpcType.h"
#include "../../../../Game/PlayerAvatar/ControlLock/PlayerAvatar_ControlLock.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"

namespace
{
    constexpr const char* ENEMY_BT_CONTROL_LOCK_TAG = "EnemyBT";
}

void GameCore::Network::ApplyPlayerControlLock(const bool isLock, NanamiEngine::Module::GameObject::IGameObject& source)
{
    if (isLock)
        PlayerAvatar::LockControlBy(source, ENEMY_BT_CONTROL_LOCK_TAG);
    else
        PlayerAvatar::UnlockControlBy(source, ENEMY_BT_CONTROL_LOCK_TAG);
}

namespace
{
    struct PlayerControlLockRpcRegistration
    {
        PlayerControlLockRpcRegistration()
        {
            GameCore::Network::PlayerControlLockRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject& networkObject, const bool isLock)
                {
                    if (const auto source = networkObject.Entity().lock())
                        GameCore::Network::ApplyPlayerControlLock(isLock, *source);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static PlayerControlLockRpcRegistration s_playerControlLockRpcRegistration;
}
