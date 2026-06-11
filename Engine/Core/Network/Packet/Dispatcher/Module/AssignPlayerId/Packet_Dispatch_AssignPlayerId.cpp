#include "Packet_Dispatch_AssignPlayerId.h"

#include "../../../../Engine_Network_INetworkSystem.h"

namespace NanamiEngine::Core::Network
{
    ReceivedAssignPlayerId::ReceivedAssignPlayerId(INetworkSystem& networkSystem)
        : PacketDispatcherBase(networkSystem, networkSystem)
        , networkSystem_(networkSystem)
    {
        
    }
    
    void ReceivedAssignPlayerId::ReceivePacket(const Packet& packet)
    {
        size_t offset = 0;
        const auto playerId = packet.Data().Read<struct PlayerId>(offset);
        networkSystem_.SetPlayerId(playerId);
    }
}
