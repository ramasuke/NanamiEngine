#pragma once
#include "../../Packet_Dispatch_PacketDispatcherBase.h"
#include "Registry/Packet_Dispatch_SyncParameter_Registry.h"

namespace NanamiEngine::Core::Network
{
    class INetworkSyncParameter;
}

namespace NanamiEngine::Core::Network
{
    class SyncParameterDispatcher final : public PacketDispatcherBase
    {
    public:
        SyncParameterDispatcher(const IPlayerIdProvider& playerIdProvider, IPacketSender& packetSender);
        void DispatchSendPacket(const INetworkSyncParameter& networkSyncParameter);
        [[nodiscard]] SyncParameterRegistry& ParameterRegistry();

    protected:
        void OnReceive(const Packet& packet) override;

        
        SyncParameterRegistry syncParameterRegistry_;
    };
}
