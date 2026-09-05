#pragma once
#include "../../../../../Engine/Core/Network/Packet/NetworkSystem_Packet.h"

namespace GameCore::Network
{
    enum class EPacketType : NanamiEngine::Core::Network::PacketType
    {
        SpawnPlayerAvatar  = 101,
        SyncAvatarState    = 102,
        SyncBehaviourTree  = 103,
        WakeUpPlayer       = 104,
    };
}
