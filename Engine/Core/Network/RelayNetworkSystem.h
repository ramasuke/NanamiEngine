#pragma once
#include <map>
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include "Engine_Network_INetworkSystem.h"
#include "EnetPeerSupport.h"
#include "Packet/NetworkSystem_Packet.h"
#include "Object/Registry/NetworkObjectInstanceRegistry.h"
#include "../../../Packages/R4/R4.h"

struct _ENetHost;
struct _ENetPeer;

namespace NanamiEngine::Core::Network
{
    /**
     * 中継サーバー(EnviroHunter-Server)経由の INetworkSystem。
     * ホストもクライアントも中継サーバーへ外向きにつなぎ、どちらになるかは中継サーバーの返事(Hosted / Joined)で決まる。
     * ホストになったら EnetUDPNetworkSystem のサーバーと同じく PlayerId を割り当て、参加と離脱を通知する
     */
    class RelayNetworkSystem final : public INetworkSystem
    {
    public:
        explicit RelayNetworkSystem(const NetworkStartSettings& settings);
        ~RelayNetworkSystem() override;
        void Update() override;
        void Send(const Packet& packet) override;
        void SendTo(PlayerId target, const Packet& packet) override;
        [[nodiscard]] std::vector<Packet> PollPackets() override;
        [[nodiscard]] INetworkObjectInstanceRegistry& GetInstanceRegistry() override;
        [[nodiscard]] ConnectionState GetConnectionState() const override;
        [[nodiscard]] std::uint16_t ListenPort() const override { return 0; }

    private:
        [[nodiscard]] PlayerId GetPlayerId() const override;
        [[nodiscard]] bool IsServer() const override;
        void SetPlayerId(PlayerId playerId) override;
        R4::Observable<PlayerId> OnConnectPlayer() override;

        void OnRelayConnected();
        void OnControlReceived(const std::uint8_t* data, std::size_t size);
        void OnGameDataReceived(const std::uint8_t* data, std::size_t size);
        void OnRelayDisconnected(std::uint32_t reason);
        void OnPeerJoined(std::uint8_t slot);
        void OnPeerLeft(std::uint8_t slot);
        /** ホストでは target の slot(全員なら TARGET_ALL)を先頭に付けて送る */
        void SendEncoded(const Packet& packet, std::optional<std::uint8_t> target);

    private:
        const std::string sessionKey_;
        ConnectionState state_ = ConnectionState::Connecting;
        bool isHost_ = false;

        _ENetHost* host_ = nullptr;
        _ENetPeer* relay_ = nullptr;

        std::queue<Packet> receivedQueue_;
        PlayerId playerId_ = PlayerId::Invalid();

        // ホストのみ使用: 次に割り当てる PlayerId(ホスト自身が 0 を取る)と、中継サーバーの slot との対応
        int nextPlayerId_ = 0;
        std::map<PlayerId, std::uint8_t> slotsByPlayer_;
        std::map<std::uint8_t, PlayerId> playersBySlot_;

        UnreliableSendThrottle unreliableThrottle_;

        R4::Subject<PlayerId> onConnectPlayer_;
        NetworkObjectInstanceRegistry instanceRegistry_;
    };
}
