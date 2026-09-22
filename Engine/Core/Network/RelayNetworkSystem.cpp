#include "RelayNetworkSystem.h"

#include <algorithm>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"
#include "Relay/RelayProtocol.h"
#include "../../Module/Exception/Engine_Module_Exception.h"
#include "../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../Module/Network/Engine_Network_PacketLog.h"
#include "../Application/Configuration/Network/ApplicationConfiguration_Network.h"
#include "Packet/Codec/Packet_Codec.h"

namespace NanamiEngine::Core::Network
{
    namespace
    {
        const char* DisconnectReasonText(const std::uint32_t reason)
        {
            switch (static_cast<NanamiRelay::DisconnectReason>(reason))
            {
            case NanamiRelay::DisconnectReason::VersionMismatch: return "中継サーバーとプロトコルのバージョンが違います";
            case NanamiRelay::DisconnectReason::BadRequest:      return "中継サーバーが要求を受け付けませんでした";
            case NanamiRelay::DisconnectReason::JoinTimeout:     return "中継サーバーへの参加要求が間に合いませんでした";
            case NanamiRelay::DisconnectReason::HostLeft:        return "ホストが抜けました";
            case NanamiRelay::DisconnectReason::ServerShutdown:  return "中継サーバーが停止しました";
            default:                                             return "切断またはタイムアウト";
            }
        }
    }

    RelayNetworkSystem::RelayNetworkSystem(const NetworkStartSettings& settings)
        : sessionKey_(settings.sessionKey)
    {
        enet_initialize();

        host_ = enet_host_create(nullptr, 1, NanamiRelay::CHANNEL_COUNT, 0, 0);
        if (!host_)
        {
            Module::LogError("RelayNetworkSystem: クライアント用のソケットを作れませんでした");
            state_ = ConnectionState::Failed;
            return;
        }

        ENetAddress address{};
        if (enet_address_set_host(&address, settings.host.address.c_str()) != 0)
        {
            Module::LogError("RelayNetworkSystem: 中継サーバーのアドレスを解決できませんでした: " + settings.host.address);
            state_ = ConnectionState::Failed;
            return;
        }
        address.port = settings.host.port;

        relay_ = enet_host_connect(host_, &address, NanamiRelay::CHANNEL_COUNT, NanamiRelay::PROTOCOL_VERSION);
        if (!relay_)
        {
            state_ = ConnectionState::Failed;
            return;
        }
        enet_peer_timeout(relay_, 0, PEER_TIMEOUT_MIN_MS, PEER_TIMEOUT_MAX_MS);
    }

    RelayNetworkSystem::~RelayNetworkSystem()
    {
        if (host_)
        {
            // 中継サーバーがタイムアウトを待たずに部屋を片付けられるよう、切断を通知してから閉じる
            DisconnectGracefully(host_, relay_);
            enet_host_destroy(host_);
        }

        enet_deinitialize();
    }

    void RelayNetworkSystem::Update()
    {
        if (!host_)
            return;

        unreliableThrottle_.Tick();

        ENetEvent event;
        while (enet_host_service(host_, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                OnRelayConnected();
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if (event.channelID == NanamiRelay::CHANNEL_CONTROL)
                    OnControlReceived(event.packet->data, event.packet->dataLength);
                else
                    OnGameDataReceived(event.packet->data, event.packet->dataLength);
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                OnRelayDisconnected(event.data);
                break;

            default:
                break;
            }
        }

        instanceRegistry_.GetTickableRegistry().TickAll();
    }

    void RelayNetworkSystem::OnRelayConnected()
    {
        const auto& appId = Application::Configuration::NetworkConfiguration::GetRelayAppId();
        const int maxClients = std::clamp(Application::Configuration::NetworkConfiguration::GetMaxClients(),
                                          1, std::min<int>(MAX_PLAYER_ID, NanamiRelay::MAX_CLIENTS_PER_ROOM));

        const auto request = NanamiRelay::EncodeJoinOrHost({ appId, sessionKey_, static_cast<std::uint8_t>(maxClients) });
        ENetPacket* packet = enet_packet_create(request.data(), request.size(), ENET_PACKET_FLAG_RELIABLE);
        if (enet_peer_send(relay_, NanamiRelay::CHANNEL_CONTROL, packet) != 0)
            enet_packet_destroy(packet);
    }

    void RelayNetworkSystem::OnControlReceived(const std::uint8_t* data, const std::size_t size)
    {
        const auto message = NanamiRelay::DecodeControl(data, size);
        if (!message)
        {
            Module::LogWarning("RelayNetworkSystem: 中継サーバーから不正な制御メッセージを受け取りました");
            return;
        }

        switch (message->type)
        {
        case NanamiRelay::ControlType::Hosted:
            {
                // EnetUDPNetworkSystem::StartServer と同じく、ホストは自分に PlayerId 0 を振って受信キューに積む
                isHost_   = true;
                playerId_ = PlayerId(nextPlayerId_++);
                state_    = ConnectionState::Connected;
                Module::Log("RelayNetworkSystem: ホストとして部屋を作りました session=" + sessionKey_);

                Packet packet = Packet::Create(DefaultPacketType::AssignPlayerId);
                packet.Data().Write(playerId_);
                receivedQueue_.push(packet);
                break;
            }
        case NanamiRelay::ControlType::Joined:
            // PlayerId はホストからの AssignPlayerId で届き、そこで Connected になる
            Module::Log("RelayNetworkSystem: 部屋に参加しました session=" + sessionKey_);
            break;
        case NanamiRelay::ControlType::PeerJoined:
            OnPeerJoined(message->slot);
            break;
        case NanamiRelay::ControlType::PeerLeft:
            OnPeerLeft(message->slot);
            break;
        default:
            break;
        }
    }

    void RelayNetworkSystem::OnGameDataReceived(const std::uint8_t* data, std::size_t size)
    {
        // ホストに届くパケットは先頭が送り主の slot
        if (isHost_)
        {
            if (size < 1)
                return;
            ++data;
            --size;
        }

        try
        {
            Packet packet = PacketCodec::Decode(data, size);
            Module::Network::LogPacket(Module::Network::PacketDirection::Receive, packet.Type(), packet.Delivery(), packet.Data().Size());
            receivedQueue_.push(packet);
        }
        catch (const Module::Exception::PacketDeserializeException& exception)
        {
            Module::LogWarning("RelayNetworkSystem: 受信パケットを破棄しました: " + std::string(exception.what()));
        }
    }

    void RelayNetworkSystem::OnRelayDisconnected(const std::uint32_t reason)
    {
        Module::Log("RelayNetworkSystem: 中継サーバーから切断されました (" + std::string(DisconnectReasonText(reason)) + ")");
        relay_ = nullptr;

        // PlayerId をもらう前に切れたなら接続失敗(中継サーバーに届かない・バージョン違いなど)
        state_ = state_ == ConnectionState::Connecting
            ? ConnectionState::Failed
            : ConnectionState::Disconnected;
    }

    void RelayNetworkSystem::OnPeerJoined(const std::uint8_t slot)
    {
        if (!isHost_)
            return;

        if (nextPlayerId_ > MAX_PLAYER_ID)
        {
            // 中継サーバー越しには相手を切断できないので、PlayerId を渡さずに相手の接続タイムアウトを待つ
            Module::LogError("RelayNetworkSystem: PlayerId を使い切ったため slot " + std::to_string(slot) + " を受け入れませんでした");
            return;
        }

        const PlayerId assignedId(nextPlayerId_++);
        slotsByPlayer_[assignedId] = slot;
        playersBySlot_[slot]       = assignedId;

        Packet packet = Packet::Create(DefaultPacketType::AssignPlayerId);
        packet.Data().Write(assignedId);
        SendTo(assignedId, packet);

        onConnectPlayer_.OnNext(assignedId);
    }

    void RelayNetworkSystem::OnPeerLeft(const std::uint8_t slot)
    {
        const auto it = playersBySlot_.find(slot);
        if (!isHost_ || it == playersBySlot_.end())
            return;

        const PlayerId leftId = it->second;
        playersBySlot_.erase(it);
        slotsByPlayer_.erase(leftId);
        Module::Log("RelayNetworkSystem: player " + leftId.ToString() + " left");

        // EnetUDPNetworkSystem::NotifyPlayerLeft と同じく、離脱者の所有物はホスト(自分)が引き継ぐ
        Packet packet = Packet::Create(DefaultPacketType::PlayerLeft);
        packet.Data().Write(leftId);
        packet.Data().Write(playerId_);
        Send(packet);
        receivedQueue_.push(packet);
    }

    void RelayNetworkSystem::Send(const Packet& packet)
    {
        if (packet.Delivery() == DeliveryMode::Unreliable && !unreliableThrottle_.IsSendAllowed())
            return;

        SendEncoded(packet, isHost_ ? std::optional(NanamiRelay::TARGET_ALL) : std::nullopt);
    }

    void RelayNetworkSystem::SendTo(const PlayerId target, const Packet& packet)
    {
        const auto it = slotsByPlayer_.find(target);
        if (!isHost_ || it == slotsByPlayer_.end())
            return;

        SendEncoded(packet, it->second);
    }

    void RelayNetworkSystem::SendEncoded(const Packet& packet, const std::optional<std::uint8_t> target)
    {
        // 未接続(中継サーバー喪失・接続前)のときは送らずに捨てる
        if (!relay_ || relay_->state != ENET_PEER_STATE_CONNECTED)
            return;

        Module::Network::LogPacket(Module::Network::PacketDirection::Send, packet.Type(), packet.Delivery(), packet.Data().Size());

        const bool        isUnreliable = packet.Delivery() == DeliveryMode::Unreliable;
        const ByteBuffer  buffer       = PacketCodec::Encode(packet);
        const std::size_t prefixSize   = target ? 1 : 0;

        ENetPacket* p = enet_packet_create(nullptr, buffer.Size() + prefixSize, isUnreliable ? 0 : ENET_PACKET_FLAG_RELIABLE);
        if (target)
            p->data[0] = *target;
        std::copy_n(buffer.Data(), buffer.Size(), p->data + prefixSize);

        const enet_uint8 channel = isUnreliable ? NanamiRelay::CHANNEL_UNRELIABLE : NanamiRelay::CHANNEL_RELIABLE;
        if (enet_peer_send(relay_, channel, p) != 0)
            enet_packet_destroy(p);
    }

    std::vector<Packet> RelayNetworkSystem::PollPackets()
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

    INetworkObjectInstanceRegistry& RelayNetworkSystem::GetInstanceRegistry()
    {
        return instanceRegistry_;
    }

    ConnectionState RelayNetworkSystem::GetConnectionState() const
    {
        return state_;
    }

    PlayerId RelayNetworkSystem::GetPlayerId() const
    {
        return playerId_;
    }

    bool RelayNetworkSystem::IsServer() const
    {
        return isHost_;
    }

    void RelayNetworkSystem::SetPlayerId(const PlayerId playerId)
    {
        playerId_ = playerId;
        if (state_ == ConnectionState::Connecting)
            state_ = ConnectionState::Connected;
    }

    R4::Observable<PlayerId> RelayNetworkSystem::OnConnectPlayer()
    {
        return onConnectPlayer_.AsObservable();
    }
}
