#include "Enemy_Behaviour_Action_UnlockPlayerControl.h"

#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "../../../../../../../../Network/Rpc/Module/PlayerControlLock/Rpc_PlayerControlLock.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::UnlockPlayerControl::DoTick(const TickContext& context)
    {
        GameCore::Network::ApplyPlayerControlLock(false, context.EnemyGameObject());

        // NOTE: 敵の BT は権威側だけで Tick されるので、他ピアの Owner にも届ける
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::PlayerControlLockRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, false);
        }

        return TickStatus::Success;
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::UnlockPlayerControl, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
