#include "Packet_Dispatch_PacketDispatcherBase.h"

#include "../../IPacketSender.h"
#include "../../IPlayerIdProvider.h"
#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Configuration/Network/ApplicationConfiguration_Network.h"

namespace NanamiEngine::Core::Network
{
    PacketDispatcherBase::PacketDispatcherBase(
        const IPlayerIdProvider& playerIdProvider,
        IPacketSender& packetSender)
        : playerIdProvider_(playerIdProvider),
          packetSender(packetSender)
    {
    }

    void PacketDispatcherBase::ReceivePacket(const Packet& packet)
    {
        if (IsServer())
        {
            if (Application::Configuration::NetworkConfiguration::GetServerType() == ServerType::Relay)
                OnServerRelayReceive(packet);
            else
                OnServerAuthoritativeReceive(packet);
        }
        else
            OnReceive(packet);
    }

    void PacketDispatcherBase::OnServerRelayReceive(const Packet& packet)
    {
        SendPacket(packet);
        OnReceive(packet);
    }

    void PacketDispatcherBase::OnServerAuthoritativeReceive(const Packet& packet)
    {
        OnReceive(packet);
    }

    void PacketDispatcherBase::OnReceive(const Packet& packet) {}

    bool PacketDispatcherBase::IsServer() const
    {
        return Application::Configuration::NetworkConfiguration::IsServer();
    }

    void PacketDispatcherBase::SendPacket(const Packet& packet) const
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
