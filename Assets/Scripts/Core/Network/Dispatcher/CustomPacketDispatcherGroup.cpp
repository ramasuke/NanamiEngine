#include "CustomPacketDispatcherGroup.h"

#include "../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"

namespace GameCore::Network
{
    CustomDispatcherGroup::CustomDispatcherGroup(
        Core::Network::DefaultPacketDispatcher& defaultDispatchers,
        Core::Network::IPacketSender& packetSender,
        Core::Network::IPlayerIdProvider& playerIdProvider)
    {
        
    }

    void CustomDispatcherGroup::DispatchReceivedPacket(const Core::Network::Packet& packet)
    {
        
    }
}
