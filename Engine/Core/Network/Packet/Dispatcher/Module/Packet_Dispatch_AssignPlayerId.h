#pragma once
#include "../Packet_Dispath_PacketDispatcherBase.h"

namespace NanamiEngine::Core::Network
{
    class INetworkSystem;
}

namespace NanamiEngine::Core::Network
{
    class ReceivedAssignPlayerId final : public PacketDispatcherBase
    {
    public:
        explicit ReceivedAssignPlayerId(INetworkSystem& networkSystem);
        
        void ReceivePacket(const Packet& packet) override;
        
    private:
        INetworkSystem& networkSystem_;
    };
}
