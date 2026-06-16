#include "EnetUDPNetworkSystem.h"

#include "../../Module/GameObject/PrefabGameObject/PrefabGameObject.h"
#include "../Application/ApplicationBase.h"
#include "../Application/Configuration/Network/ApplicationConfiguration_Network.h"
#include "Packet/Codec/Packet_Codec.h"

namespace NanamiEngine::Core::Network
{
    EnetUDPNetworkSystem::EnetUDPNetworkSystem()
    {
        enet_initialize();
        
        if (Application::Configuration::NetworkConfiguration::IsServer())
        {
            ENetAddress address{};
            address.host = ENET_HOST_ANY;
            address.port = PORT_ADDRESS;

            host_ = enet_host_create(&address, MAX_CLIENTS, 2, 0, 0);

            for (int i = 0; i < MAX_CLIENTS; ++i)
            {
                availableIds_.push(i);
            }

            //Clientと同樣にPlayerIdを設定するPacketを自分自身に送信
            Packet packet = Packet::Create(DefaultPacketType::AssignPlayerId);
            packet.Data().Write(playerId_);
            receivedQueue_.push(packet);
        }
        else
        {
            host_ = enet_host_create(nullptr, 1, 2, 0, 0);

            ENetAddress address{};
            enet_address_set_host(&address, Application::Configuration::NetworkConfiguration::GetServerAddress());
            address.port = PORT_ADDRESS;

            peer_ = enet_host_connect(host_, &address, 2, 0);
        }
    }

    EnetUDPNetworkSystem::~EnetUDPNetworkSystem()
    {
        if (host_)
            enet_host_destroy(host_);

        enet_deinitialize();
    }

    void EnetUDPNetworkSystem::Update()
    {
        ENetEvent event;

        while (enet_host_service(host_, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                {
                    if (Application::Configuration::NetworkConfiguration::IsServer())
                    {
                        static int nextId = 0;
                        int assignedId = nextId++;

                        event.peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(assignedId));

                        //設定されたIDを通知するパケット
                        Packet p = Packet::Create(DefaultPacketType::AssignPlayerId);
                        p.Data().Write(assignedId);

                        SendTo(event.peer, p);
                    }
                    else
                    {
                        
                    }
                    break;
                }

            case ENET_EVENT_TYPE_RECEIVE:
                {
                    Packet p = PacketCodec::Decode(
                        event.packet->data,
                        event.packet->dataLength
                    );

                    receivedQueue_.push(p);

                    enet_packet_destroy(event.packet);
                    break;
                }

            case ENET_EVENT_TYPE_DISCONNECT:
                // TODO: 切断イベント
                break;

            default:
                break;
            }
        }
    }

    void EnetUDPNetworkSystem::Send(const Packet& packet)
    {
        const ByteBuffer buffer = PacketCodec::Encode(packet);
        ENetPacket* p = enet_packet_create(
            buffer.Data(),
            buffer.Size(),
            ENET_PACKET_FLAG_RELIABLE
        );

        if (Application::Configuration::NetworkConfiguration::IsServer())
            enet_host_broadcast(host_, 0, p);
        else
        {
            assert(peer_);
            enet_peer_send(peer_, 0, p);
        }
    }

    void EnetUDPNetworkSystem::SendTo(ENetPeer* target, const Packet& packet)
    {
        if (!target)
            return;

        ByteBuffer buffer = PacketCodec::Encode(packet);

        ENetPacket* p = enet_packet_create(
            buffer.Data(),
            buffer.Size(),
            ENET_PACKET_FLAG_RELIABLE
        );

        enet_peer_send(target, 0, p);
    }

    PlayerId EnetUDPNetworkSystem::GetPlayerId() const
    {
        return playerId_;
    }

    void EnetUDPNetworkSystem::SetPlayerId(const PlayerId playerId)
    {
        playerId_ = playerId;
    }

    std::vector<Packet> EnetUDPNetworkSystem::PollPackets()
    {
        std::vector<Packet> result;
        result.reserve(receivedQueue_.size());

        while (!receivedQueue_.empty())
        {
            result.push_back(std::move(receivedQueue_.front()));
            receivedQueue_.pop();
        }

        return result;
    }
}
