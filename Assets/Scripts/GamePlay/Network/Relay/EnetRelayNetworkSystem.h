#pragma once
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "Engine/Core/Network/Engine_Network_INetworkSystem.h"
#include "Engine/Core/Network/EnetPeerSupport.h"
#include "Engine/Core/Network/Packet/NetworkSystem_Packet.h"
#include "Engine/Core/Network/Object/Registry/NetworkObjectInstanceRegistry.h"
#include "Packages/R4/R4.h"
#include "RelayRoom.h"
#include "RelayServerSettings.h"

struct _ENetHost;
struct _ENetPeer;

namespace GamePlay::Network
{
    /**
     * 中継サーバー(EnviroHunter-Server)経由の INetworkSystem。
     * ホストもクライアントも中継サーバーへ外向きにつなぎ、どちらになるかは中継サーバーの返事(Hosted / Joined)で決まる。
     * ホストになったら EnetUDPNetworkSystem のサーバーと同じく PlayerId を割り当て、参加と離脱を通知する。
     * 非公開部屋(RelayRoom::Create / Join)では、部屋のコードと断られた理由を status に書く
     */
    class EnetRelayNetworkSystem final : public NanamiEngine::Core::Network::INetworkSystem
    {
    public:
        EnetRelayNetworkSystem(const RelayServerSettings& settings, std::string sessionKey, RelayRoom room,
                               std::shared_ptr<RelayRoomStatus> status);
        ~EnetRelayNetworkSystem() override;
        void Update() override;
        void Send(const NanamiEngine::Core::Network::Packet& packet) override;
        void SendTo(NanamiEngine::Core::Network::PlayerId target, const NanamiEngine::Core::Network::Packet& packet) override;
        [[nodiscard]] std::vector<NanamiEngine::Core::Network::Packet> PollPackets() override;
        [[nodiscard]] NanamiEngine::Core::Network::INetworkObjectInstanceRegistry& GetInstanceRegistry() override;
        [[nodiscard]] NanamiEngine::Core::Network::ConnectionState GetConnectionState() const override;
        [[nodiscard]] std::uint16_t ListenPort() const override { return 0; }

    private:
        [[nodiscard]] NanamiEngine::Core::Network::PlayerId GetPlayerId() const override;
        [[nodiscard]] bool IsServer() const override;
        void SetPlayerId(NanamiEngine::Core::Network::PlayerId playerId) override;
        NanamiEngine::R4::Observable<NanamiEngine::Core::Network::PlayerId> OnConnectPlayer() override;

        void OnRelayConnected();
        /** ホストとして部屋に入った。自分に PlayerId 0 を振る */
        void BecomeHost();
        void OnControlReceived(const std::uint8_t* data, std::size_t size);
        void OnGameDataReceived(const std::uint8_t* data, std::size_t size);
        void OnRelayDisconnected(std::uint32_t reason);
        void OnPeerJoined(std::uint8_t slot);
        void OnPeerLeft(std::uint8_t slot);
        /** ホストでは target の slot(全員なら TARGET_ALL)を先頭に付けて送る */
        void SendEncoded(const NanamiEngine::Core::Network::Packet& packet, std::optional<std::uint8_t> target);

    private:
        const std::string sessionKey_;
        const std::string appId_;
        const RelayRoom   room_;
        const std::shared_ptr<RelayRoomStatus> status_;
        NanamiEngine::Core::Network::ConnectionState state_ = NanamiEngine::Core::Network::ConnectionState::Connecting;
        bool isHost_ = false;

        _ENetHost* host_ = nullptr;
        _ENetPeer* relay_ = nullptr;

        std::queue<NanamiEngine::Core::Network::Packet> receivedQueue_;
        NanamiEngine::Core::Network::PlayerId playerId_ = NanamiEngine::Core::Network::PlayerId::Invalid();

        // ホストのみ使用: 次に割り当てる PlayerId(ホスト自身が 0 を取る)と、中継サーバーの slot との対応
        int nextPlayerId_ = 0;
        std::map<NanamiEngine::Core::Network::PlayerId, std::uint8_t> slotsByPlayer_;
        std::map<std::uint8_t, NanamiEngine::Core::Network::PlayerId> playersBySlot_;

        NanamiEngine::Core::Network::UnreliableSendThrottle unreliableThrottle_;

        NanamiEngine::R4::Subject<NanamiEngine::Core::Network::PlayerId> onConnectPlayer_;
        NanamiEngine::Core::Network::NetworkObjectInstanceRegistry instanceRegistry_;
    };
}
