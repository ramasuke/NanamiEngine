#include "CustomPacketDispatcherBase.h"

namespace GameCore::Network
{
    CustomDispatcherBase::CustomDispatcherBase(
        Core::Network::DefaultPacketDispatcher& defaultDispatchers,
        const Core::Network::IPlayerIdProvider& playerIdProvider,
        Core::Network::IPacketSender& packetSender)
        : PacketDispatcherBase(playerIdProvider, packetSender)
        , defaultDispatchers_(defaultDispatchers)
        , packetSender_(packetSender)
    {
    }
}
