#include "Packet_Dispatch_SpawnPlayer.h"

namespace GameCore::Network
{
    SpawnPlayerDispatcher::SpawnPlayerDispatcher(
        Core::Network::DefaultPacketDispatcher& defaultDispatchers,
        const Core::Network::IPlayerIdProvider& playerIdProvider,
        Core::Network::IPacketSender& packetSender)
            : CustomDispatcherBase(defaultDispatchers, playerIdProvider, packetSender)
    {
    }
}
