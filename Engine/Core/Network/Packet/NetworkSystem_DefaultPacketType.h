#pragma once
#include "NetworkSystem_PacketType.h"

namespace NanamiEngine::Core::Network
{
    enum class DefaultPacketType : PacketType
    {
        AssignPlayerId     = 0,
        SpawnNetworkObject = 1,
        SyncTransform      = 2,
        SyncAnimation      = 3,
        SyncParameter      = 4,
        Rpc                = 5,
        PlayerLeft         = 6,
        OwnershipSnapshot  = 7,
    };
}
