#include "CustomPacketDispatcherGroup.h"

namespace GamePlay::Network
{
    CustomDispatcherGroup::CustomDispatcherGroup(
        Core::Network::IPacketSender& packetSender,
        Core::Network::IPlayerIdProvider& playerIdProvider)
    {
        
    }

    void CustomDispatcherGroup::DispatchReceivedPacket(const Core::Network::Packet& packet)
    {
        
    }
}
