#pragma once
#include <map>
#include <vector>
#include <queue>

#include "Engine_Network_INetworkSystem.h"
#include "EnetPeerSupport.h"
#include "Packet/NetworkSystem_Packet.h"
#include "Object/Registry/NetworkObjectInstanceRegistry.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"
#include "../../../Packages/R4/R4.h"

struct _ENetHost;
struct _ENetPeer;

namespace NanamiEngine::Core::Network
{
    constexpr auto PORT_ADDRESS = 1234;

    class EnetUDPNetworkSystem final : public INetworkSystem
    {
    public:
        explicit EnetUDPNetworkSystem(const NetworkStartSettings& settings);
        ~EnetUDPNetworkSystem() override;
        void Update() override;
        void Send(const Packet& packet) override;
        void SendTo(PlayerId target, const Packet& packet) override;
        [[nodiscard]] std::vector<Packet> PollPackets() override;
        [[nodiscard]] INetworkObjectInstanceRegistry& GetInstanceRegistry() override;
        [[nodiscard]] ConnectionState GetConnectionState() const override;
        [[nodiscard]] std::uint16_t ListenPort() const override;

    private:
        void StartServer();
        void StartClient(const HostEndpoint& host);
        [[nodiscard]] PlayerId GetPlayerId() const override;
        [[nodiscard]] bool IsServer() const override;
        /** ホストのみ: 離脱者の所有物を自分が引き継ぐ PlayerLeft を全員へ配り、自分の受信キューにも積む */
        void NotifyPlayerLeft(PlayerId leftId);
        void SetPlayerId(PlayerId playerId) override;
        R4::Observable<PlayerId> OnConnectPlayer() override;
        void SendToPeer(_ENetPeer* target, const Packet& packet);

    private:
        const Mode mode_;
        ConnectionState state_ = ConnectionState::Connecting;

        _ENetHost* host_ = nullptr;
        _ENetPeer* peer_ = nullptr;

        std::queue<Packet> receivedQueue_;
        PlayerId playerId_ = PlayerId::Invalid();

        // ホストのみ使用: 次に割り当てる PlayerId(ホスト自身が 0 を取る)と接続中 peer の一覧
        int nextPlayerId_ = 0;
        std::map<PlayerId, _ENetPeer*> peers_;

        UnreliableSendThrottle unreliableThrottle_;

        R4::Subject<PlayerId> onConnectPlayer_;
        NetworkObjectInstanceRegistry instanceRegistry_;
    };
}