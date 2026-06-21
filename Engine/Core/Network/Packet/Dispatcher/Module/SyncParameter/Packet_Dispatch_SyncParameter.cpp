#include "Packet_Dispatch_SyncParameter.h"
#include "../../../../Object/SyncParameter/Network_INetworkSyncParameter.h"

namespace NanamiEngine::Core::Network
{
    SyncParameterDispatcher::SyncParameterDispatcher(
        const IPlayerIdProvider& playerIdProvider,
        IPacketSender& packetSender)
        : PacketDispatcherBase(playerIdProvider, packetSender)
    {
    }

    void SyncParameterDispatcher::DispatchSendPacket(const INetworkSyncParameter& networkSyncParameter)
    {
        Packet packet = Packet::Create(DefaultPacketType::SyncParameter);
        networkSyncParameter.WriteTo(packet.Data());
        SendPacket(packet);
    }

    SyncParameterRegistry& SyncParameterDispatcher::ParameterRegistry()
    {
        return syncParameterRegistry_;
    }

    void SyncParameterDispatcher::OnReceive(const Packet& packet)
    {
        size_t offset = 0;
        const ParameterId id(packet.Data().Read<uint64_t>(offset));
        if (auto* param = syncParameterRegistry_.Find(id))
            param->SyncReceiveParam(packet.Data(), offset);
    }
}
