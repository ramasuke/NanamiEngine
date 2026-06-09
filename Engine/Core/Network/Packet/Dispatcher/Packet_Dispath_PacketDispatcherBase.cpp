#include "Packet_Dispath_PacketDispatcherBase.h"

#include "../../IPacketSender.h"
#include "../../IPlayerIdProvider.h"
#include "../../../Application/ApplicationBase.h"

namespace NanamiEngine::Core::Network
{
    PacketDispatcherBase::PacketDispatcherBase(
        const IPlayerIdProvider& playerIdProvider,
        IPacketSender& packetSender)
        : playerIdProvider_(playerIdProvider),
          packetSender(packetSender)
    {
    }

    void PacketDispatcherBase::SystemSendPacket(const Packet& packet) const
    {
        packetSender.Send(packet);
    }

    PrefabObjectRegistry& PacketDispatcherBase::NetworkObjectRegistry() const
    {
        return Application::ApplicationBase::NetworkPrefabObjectRegistry();
    }

    PlayerId PacketDispatcherBase::PlayerId() const
    {
        return playerIdProvider_.GetPlayerId();
    }
}
