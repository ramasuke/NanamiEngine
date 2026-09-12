#include "EnetUDPNetworkSystem.h"

#include "../../Module/Exception/Engine_Module_Exception.h"
#include "../../Module/GameObject/PrefabGameObject/PrefabGameObject.h"
#include "../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../Module/Network/Engine_Network_PacketLog.h"
#include "../Application/ApplicationBase.h"
#include "../Application/Configuration/Network/ApplicationConfiguration_Network.h"
#include "../Application/Time/Time.h"
#include "Packet/Codec/Packet_Codec.h"

namespace NanamiEngine::Core::Network
{
    namespace
    {
        void* EncodePeerData(const PlayerId id)
        {
            return reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<uint8_t>(id.Value())) + 1);
        }

        PlayerId DecodePeerData(const void* data)
        {
            const auto raw = reinterpret_cast<uintptr_t>(data);
            return raw == 0 ? PlayerId::Invalid() : PlayerId(static_cast<int>(raw - 1));
        }
    }

    EnetUDPNetworkSystem::EnetUDPNetworkSystem()
    {
        enet_initialize();
        
        if (Application::Configuration::NetworkConfiguration::IsServer())
        {
            ENetAddress address{};
            address.host = ENET_HOST_ANY;
            address.port = PORT_ADDRESS;

            const int maxClients = Application::Configuration::NetworkConfiguration::GetMaxClients();
            host_ = enet_host_create(&address, maxClients, 2, 0, 0);
            
            playerId_ = PlayerId(nextPlayerId_++);
            
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
            if (peer_)
                enet_peer_timeout(peer_, 0, PEER_TIMEOUT_MIN_MS, PEER_TIMEOUT_MAX_MS);
        }
    }

    EnetUDPNetworkSystem::~EnetUDPNetworkSystem()
    {
        if (host_)
        {
            // クライアントは切断を通知してから閉じる(ホスト側がタイムアウトを待たずに DISCONNECT を受け取れる)
            if (peer_ && peer_->state == ENET_PEER_STATE_CONNECTED)
            {
                enet_peer_disconnect(peer_, 0);

                ENetEvent event;
                const enet_uint32 start = enet_time_get();
                while (enet_time_get() - start < GRACEFUL_DISCONNECT_WAIT_MS
                       && enet_host_service(host_, &event, 10) >= 0)
                {
                    if (event.type == ENET_EVENT_TYPE_RECEIVE)
                        enet_packet_destroy(event.packet);
                    else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                        break;
                }
            }
            enet_host_destroy(host_);
        }

        enet_deinitialize();
    }

    void EnetUDPNetworkSystem::Update()
    {
        const float sendInterval = 1.0f / static_cast<float>(Application::Configuration::NetworkConfiguration::GetUnreliableSendRate());
        unreliableAccumulator_ += Time::DeltaTime();
        unreliableSendAllowed_ = unreliableAccumulator_ >= sendInterval;
        if (unreliableSendAllowed_)
            unreliableAccumulator_ = 0.0f;

        ENetEvent event;

        while (enet_host_service(host_, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                {
                    if (Application::Configuration::NetworkConfiguration::IsServer())
                    {
                        if (nextPlayerId_ > MAX_PLAYER_ID)
                        {
                            Module::LogError("EnetUDPNetworkSystem: PlayerId を使い切ったため接続を拒否しました");
                            enet_peer_disconnect_now(event.peer, 0);
                            break;
                        }

                        const PlayerId assignedId(nextPlayerId_++);
                        peers_[assignedId] = event.peer;
                        event.peer->data = EncodePeerData(assignedId);
                        enet_peer_timeout(event.peer, 0, PEER_TIMEOUT_MIN_MS, PEER_TIMEOUT_MAX_MS);

                        //設定されたIDを通知するパケット
                        Packet p = Packet::Create(DefaultPacketType::AssignPlayerId);
                        p.Data().Write(assignedId);

                        SendTo(event.peer, p);

                        onConnectPlayer_.get_subscriber().on_next(&event);
                    }
                    else
                    {

                    }
                    break;
                }

            case ENET_EVENT_TYPE_RECEIVE:
                {
                    try
                    {
                        Packet p = PacketCodec::Decode(
                            event.packet->data,
                            event.packet->dataLength
                        );

                        Module::Network::LogPacket(Module::Network::PacketDirection::Receive, p.Type(), p.Delivery(), p.Data().Size());
                        receivedQueue_.push(p);
                    }
                    catch (const Module::Exception::PacketDeserializeException& exception)
                    {
                        // ヘッダが不正な長さのパケットは捨てて受信ループを続ける
                        Module::LogWarning("EnetUDPNetworkSystem: 受信パケットを破棄しました: " + std::string(exception.what()));
                    }

                    enet_packet_destroy(event.packet);
                    break;
                }

            case ENET_EVENT_TYPE_DISCONNECT:
                {
                    // event.data == 0 ならタイムアウト(相手が enet_peer_disconnect を呼ばずに消えた)
                    const PlayerId leftId = DecodePeerData(event.peer->data);
                    Module::Log("Disconnect peer=" + leftId.ToString() + " data=" + std::to_string(event.data));

                    if (Application::Configuration::NetworkConfiguration::IsServer())
                    {
                        peers_.erase(leftId);
                        NotifyPlayerLeft(leftId);
                    }

                    event.peer->data = nullptr;
                    break;
                }

            default:
                break;
            }
        }

        instanceRegistry_.GetTickableRegistry().TickAll();
    }

    void EnetUDPNetworkSystem::Send(const Packet& packet)
    {
        const bool isUnreliable = packet.Delivery() == DeliveryMode::Unreliable;
        if (isUnreliable && !unreliableSendAllowed_)
            return;

        Module::Network::LogPacket(Module::Network::PacketDirection::Send, packet.Type(), packet.Delivery(), packet.Data().Size());

        const ByteBuffer  buffer = PacketCodec::Encode(packet);
        const enet_uint32 flag   = isUnreliable ? 0 : ENET_PACKET_FLAG_RELIABLE;
        const enet_uint8  ch     = isUnreliable ? 1 : 0;

        ENetPacket* p = enet_packet_create(buffer.Data(), buffer.Size(), flag);

        if (Application::Configuration::NetworkConfiguration::IsServer())
        {
            enet_host_broadcast(host_, ch, p);
            return;
        }

        // 未接続(ホスト喪失・接続前)のときは送らずに捨てる。enet_peer_send は失敗時にパケットを解放しない
        const bool sent = peer_ && peer_->state == ENET_PEER_STATE_CONNECTED && enet_peer_send(peer_, ch, p) == 0;
        if (!sent)
            enet_packet_destroy(p);
    }

    void EnetUDPNetworkSystem::SendTo(ENetPeer* target, const Packet& packet)
    {
        if (!target)
            return;

        Module::Network::LogPacket(Module::Network::PacketDirection::Send, packet.Type(), packet.Delivery(), packet.Data().Size());

        const bool isUnreliable = packet.Delivery() == DeliveryMode::Unreliable;
        const ByteBuffer buffer = PacketCodec::Encode(packet);

        ENetPacket* p = enet_packet_create(
            buffer.Data(),
            buffer.Size(),
            isUnreliable ? 0 : ENET_PACKET_FLAG_RELIABLE
        );

        if (target->state != ENET_PEER_STATE_CONNECTED || enet_peer_send(target, isUnreliable ? 1 : 0, p) != 0)
            enet_packet_destroy(p);
    }

    void EnetUDPNetworkSystem::NotifyPlayerLeft(const PlayerId leftId)
    {
        if (leftId == PlayerId::Invalid())
            return;

        // 離脱者の所有物はホスト(自分)が引き継ぐ。全クライアントへ配り、自分も同じディスパッチ経路で処理する
        Packet packet = Packet::Create(DefaultPacketType::PlayerLeft);
        packet.Data().Write(leftId);
        packet.Data().Write(playerId_);
        Send(packet);
        receivedQueue_.push(packet);
    }

    PlayerId EnetUDPNetworkSystem::GetPlayerId() const
    {
        return playerId_;
    }

    void EnetUDPNetworkSystem::SetPlayerId(const PlayerId playerId)
    {
        playerId_ = playerId;
    }

    rxcpp::observable<ENetEvent*> EnetUDPNetworkSystem::OnConnectPlayer()
    {
        return onConnectPlayer_.get_observable();
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

    INetworkObjectInstanceRegistry& EnetUDPNetworkSystem::GetInstanceRegistry()
    {
        return instanceRegistry_;
    }
}
