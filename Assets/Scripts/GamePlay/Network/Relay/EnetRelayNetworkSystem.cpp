#include "EnetRelayNetworkSystem.h"

#include <algorithm>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"
#include "RelayProtocol.h"
#include "Engine/Core/Application/Configuration/Network/ApplicationConfiguration_Network.h"
#include "Engine/Core/Network/Packet/Codec/Packet_Codec.h"
#include "Engine/Module/Exception/Engine_Module_Exception.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Network/Engine_Network_PacketLog.h"

namespace GamePlay::Network
{
    namespace
    {
        using NanamiEngine::Core::Network::ConnectionState;
        using NanamiEngine::Core::Network::DefaultPacketType;
        using NanamiEngine::Core::Network::Packet;
        using NanamiEngine::Core::Network::PlayerId;

        const char* DisconnectReasonText(const std::uint32_t reason)
        {
            switch (static_cast<NanamiRelay::DisconnectReason>(reason))
            {
            case NanamiRelay::DisconnectReason::VersionMismatch: return "中継サーバーとプロトコルのバージョンが違います";
            case NanamiRelay::DisconnectReason::BadRequest:      return "中継サーバーが要求を受け付けませんでした";
            case NanamiRelay::DisconnectReason::JoinTimeout:     return "中継サーバーへの参加要求が間に合いませんでした";
            case NanamiRelay::DisconnectReason::HostLeft:        return "ホストが抜けました";
            case NanamiRelay::DisconnectReason::ServerShutdown:  return "中継サーバーが停止しました";
            case NanamiRelay::DisconnectReason::RoomNotFound:    return "その番号の部屋は見つかりませんでした";
            case NanamiRelay::DisconnectReason::RoomFull:        return "その部屋はもう満員です";
            case NanamiRelay::DisconnectReason::SessionMismatch: return "その部屋は別のステージに出ています";
            default:                                             return "切断またはタイムアウト";
            }
        }
    }

    EnetRelayNetworkSystem::EnetRelayNetworkSystem(const RelayServerSettings& settings, std::string sessionKey, RelayRoom room,
                                                   std::shared_ptr<RelayRoomStatus> status)
        : sessionKey_(std::move(sessionKey))
        , appId_(settings.appId)
        , room_(std::move(room))
        , status_(std::move(status))
    {
        enet_initialize();

        host_ = enet_host_create(nullptr, 1, NanamiRelay::CHANNEL_COUNT, 0, 0);
        if (!host_)
        {
            NanamiEngine::Module::LogError("EnetRelayNetworkSystem: クライアント用のソケットを作れませんでした");
            state_ = ConnectionState::Failed;
            return;
        }

        ENetAddress address{};
        if (enet_address_set_host(&address, settings.address.c_str()) != 0)
        {
            NanamiEngine::Module::LogError("EnetRelayNetworkSystem: 中継サーバーのアドレスを解決できませんでした: " + settings.address);
            state_ = ConnectionState::Failed;
            return;
        }
        address.port = settings.port;

        relay_ = enet_host_connect(host_, &address, NanamiRelay::CHANNEL_COUNT, NanamiRelay::PROTOCOL_VERSION);
        if (!relay_)
        {
            state_ = ConnectionState::Failed;
            return;
        }
        enet_peer_timeout(relay_, 0, NanamiEngine::Core::Network::PEER_TIMEOUT_MIN_MS, NanamiEngine::Core::Network::PEER_TIMEOUT_MAX_MS);
    }

    EnetRelayNetworkSystem::~EnetRelayNetworkSystem()
    {
        if (host_)
        {
            // 中継サーバーがタイムアウトを待たずに部屋を片付けられるよう、切断を通知してから閉じる
            NanamiEngine::Core::Network::DisconnectGracefully(host_, relay_);
            enet_host_destroy(host_);
        }

        enet_deinitialize();
    }

    void EnetRelayNetworkSystem::Update()
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

    void EnetRelayNetworkSystem::OnRelayConnected()
    {
        const int maxClients = std::clamp(NanamiEngine::Core::Application::Configuration::NetworkConfiguration::GetMaxClients(),
                                          1, std::min<int>(NanamiEngine::Core::Network::MAX_PLAYER_ID, NanamiRelay::MAX_CLIENTS_PER_ROOM));

        const NanamiRelay::JoinOrHostRequest request{ appId_, sessionKey_, static_cast<std::uint8_t>(maxClients), room_.code };
        std::vector<std::uint8_t> bytes;
        switch (room_.mode)
        {
        case RelayRoom::Mode::Public: bytes = NanamiRelay::EncodeJoinOrHost(request); break;
        case RelayRoom::Mode::Create: bytes = NanamiRelay::EncodeCreateRoom(request); break;
        case RelayRoom::Mode::Join:   bytes = NanamiRelay::EncodeJoinRoom(request);   break;
        }

        ENetPacket* packet = enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE);
        if (enet_peer_send(relay_, NanamiRelay::CHANNEL_CONTROL, packet) != 0)
            enet_packet_destroy(packet);
    }

    void EnetRelayNetworkSystem::OnControlReceived(const std::uint8_t* data, const std::size_t size)
    {
        const auto message = NanamiRelay::DecodeControl(data, size);
        if (!message)
        {
            NanamiEngine::Module::LogWarning("EnetRelayNetworkSystem: 中継サーバーから不正な制御メッセージを受け取りました");
            return;
        }

        switch (message->type)
        {
        case NanamiRelay::ControlType::Hosted:
            NanamiEngine::Module::Log("EnetRelayNetworkSystem: ホストとして部屋を作りました session=" + sessionKey_);
            BecomeHost();
            break;
        case NanamiRelay::ControlType::RoomCreated:
            NanamiEngine::Module::Log("EnetRelayNetworkSystem: 非公開の部屋を作りました session=" + sessionKey_ + " code=" + message->roomCode);
            status_->code = message->roomCode;
            BecomeHost();
            break;
        case NanamiRelay::ControlType::Joined:
            // PlayerId はホストからの AssignPlayerId で届き、そこで Connected になる
            NanamiEngine::Module::Log("EnetRelayNetworkSystem: 部屋に参加しました session=" + sessionKey_);
            if (room_.mode == RelayRoom::Mode::Join)
                status_->code = room_.code;
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

    void EnetRelayNetworkSystem::BecomeHost()
    {
        // EnetUDPNetworkSystem のサーバーと同じく、ホストは自分に PlayerId 0 を振って受信キューに積む
        isHost_   = true;
        playerId_ = PlayerId(nextPlayerId_++);
        state_    = ConnectionState::Connected;

        Packet packet = Packet::Create(DefaultPacketType::AssignPlayerId);
        packet.Data().Write(playerId_);
        receivedQueue_.push(packet);
    }

    void EnetRelayNetworkSystem::OnGameDataReceived(const std::uint8_t* data, std::size_t size)
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
            Packet packet = NanamiEngine::Core::Network::PacketCodec::Decode(data, size);
            NanamiEngine::Module::Network::LogPacket(NanamiEngine::Module::Network::PacketDirection::Receive, packet.Type(), packet.Delivery(), packet.Data().Size());
            receivedQueue_.push(packet);
        }
        catch (const NanamiEngine::Module::Exception::PacketDeserializeException& exception)
        {
            NanamiEngine::Module::LogWarning("EnetRelayNetworkSystem: 受信パケットを破棄しました: " + std::string(exception.what()));
        }
    }

    void EnetRelayNetworkSystem::OnRelayDisconnected(const std::uint32_t reason)
    {
        NanamiEngine::Module::Log("EnetRelayNetworkSystem: 中継サーバーから切断されました (" + std::string(DisconnectReasonText(reason)) + ")");
        relay_ = nullptr;

        // PlayerId をもらう前に切れたなら接続失敗(中継サーバーに届かない・バージョン違い・部屋に断られたなど)
        if (state_ == ConnectionState::Connecting)
        {
            state_ = ConnectionState::Failed;
            status_->failure = DisconnectReasonText(reason);
        }
        else
        {
            state_ = ConnectionState::Disconnected;
        }
    }

    void EnetRelayNetworkSystem::OnPeerJoined(const std::uint8_t slot)
    {
        if (!isHost_)
            return;

        if (nextPlayerId_ > NanamiEngine::Core::Network::MAX_PLAYER_ID)
        {
            // 中継サーバー越しには相手を切断できないので、PlayerId を渡さずに相手の接続タイムアウトを待つ
            NanamiEngine::Module::LogError("EnetRelayNetworkSystem: PlayerId を使い切ったため slot " + std::to_string(slot) + " を受け入れませんでした");
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

    void EnetRelayNetworkSystem::OnPeerLeft(const std::uint8_t slot)
    {
        const auto it = playersBySlot_.find(slot);
        if (!isHost_ || it == playersBySlot_.end())
            return;

        const PlayerId leftId = it->second;
        playersBySlot_.erase(it);
        slotsByPlayer_.erase(leftId);
        NanamiEngine::Module::Log("EnetRelayNetworkSystem: player " + leftId.ToString() + " left");

        // EnetUDPNetworkSystem::NotifyPlayerLeft と同じく、離脱者の所有物はホスト(自分)が引き継ぐ
        Packet packet = Packet::Create(DefaultPacketType::PlayerLeft);
        packet.Data().Write(leftId);
        packet.Data().Write(playerId_);
        Send(packet);
        receivedQueue_.push(packet);
    }

    void EnetRelayNetworkSystem::Send(const Packet& packet)
    {
        if (packet.Delivery() == NanamiEngine::Core::Network::DeliveryMode::Unreliable && !unreliableThrottle_.IsSendAllowed())
            return;

        SendEncoded(packet, isHost_ ? std::optional(NanamiRelay::TARGET_ALL) : std::nullopt);
    }

    void EnetRelayNetworkSystem::SendTo(const PlayerId target, const Packet& packet)
    {
        const auto it = slotsByPlayer_.find(target);
        if (!isHost_ || it == slotsByPlayer_.end())
            return;

        SendEncoded(packet, it->second);
    }

    void EnetRelayNetworkSystem::SendEncoded(const Packet& packet, const std::optional<std::uint8_t> target)
    {
        // 未接続(中継サーバー喪失・接続前)のときは送らずに捨てる
        if (!relay_ || relay_->state != ENET_PEER_STATE_CONNECTED)
            return;

        NanamiEngine::Module::Network::LogPacket(NanamiEngine::Module::Network::PacketDirection::Send, packet.Type(), packet.Delivery(), packet.Data().Size());

        const bool        isUnreliable = packet.Delivery() == NanamiEngine::Core::Network::DeliveryMode::Unreliable;
        const auto        buffer       = NanamiEngine::Core::Network::PacketCodec::Encode(packet);
        const std::size_t prefixSize   = target ? 1 : 0;

        ENetPacket* p = enet_packet_create(nullptr, buffer.Size() + prefixSize, isUnreliable ? 0 : ENET_PACKET_FLAG_RELIABLE);
        if (target)
            p->data[0] = *target;
        std::copy_n(buffer.Data(), buffer.Size(), p->data + prefixSize);

        const enet_uint8 channel = isUnreliable ? NanamiRelay::CHANNEL_UNRELIABLE : NanamiRelay::CHANNEL_RELIABLE;
        if (enet_peer_send(relay_, channel, p) != 0)
            enet_packet_destroy(p);
    }

    std::vector<Packet> EnetRelayNetworkSystem::PollPackets()
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

    NanamiEngine::Core::Network::INetworkObjectInstanceRegistry& EnetRelayNetworkSystem::GetInstanceRegistry()
    {
        return instanceRegistry_;
    }

    ConnectionState EnetRelayNetworkSystem::GetConnectionState() const
    {
        return state_;
    }

    PlayerId EnetRelayNetworkSystem::GetPlayerId() const
    {
        return playerId_;
    }

    bool EnetRelayNetworkSystem::IsServer() const
    {
        return isHost_;
    }

    void EnetRelayNetworkSystem::SetPlayerId(const PlayerId playerId)
    {
        playerId_ = playerId;
        if (state_ == ConnectionState::Connecting)
            state_ = ConnectionState::Connected;
    }

    NanamiEngine::R4::Observable<PlayerId> EnetRelayNetworkSystem::OnConnectPlayer()
    {
        return onConnectPlayer_.AsObservable();
    }
}
