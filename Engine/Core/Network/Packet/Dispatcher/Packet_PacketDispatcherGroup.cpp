#include "Packet_PacketDispatcherGroup.h"

#include "../../Engine_Network_INetworkSystem.h"

namespace NanamiEngine::Core::Network
{
    DefaultPacketDispatcher::DefaultPacketDispatcher(
        INetworkSystem& networkSystem)
        : receivedAssignPlayerId_(networkSystem)
        , spawnNetworkObject_(networkSystem, networkSystem)
    {
    }

    void DefaultPacketDispatcher::DispatchReceivedPacket(const Packet& packet)
    {
        auto defaultedType = static_cast<DefaultPacketType>(packet.Type());
        switch (defaultedType)
        {
        case DefaultPacketType::AssignPlayerId:
            receivedAssignPlayerId_.ReceivePacket(packet);
            break;
        case DefaultPacketType::SpawnNetworkObject:
            spawnNetworkObject_.ReceivePacket(packet);
            break;
        }
    }
}
