#pragma once
#include "Module/AssignPlayerId/Packet_Dispatch_AssignPlayerId.h"
#include "Module/SpawnNetworkObject/Packet_Dispatch_SpawnNetworkObject.h"
#include "../../Object/Registry/NetworkObjectInstanceRegistry.h"

namespace NanamiEngine::Core::Network
{
    class DefaultPacketDispatcher final
    {
    public:
        explicit DefaultPacketDispatcher(
            INetworkSystem& networkSystem);
        [[nodiscard]] SpawnNetworkObject& Spawn() { return spawnNetworkObject_; }

        void DispatchReceivedPacket(const Packet& packet);

    private:
        NetworkObjectInstanceRegistry instanceRegistry_;
        ReceivedAssignPlayerId receivedAssignPlayerId_;
        SpawnNetworkObject spawnNetworkObject_;
    };
}
