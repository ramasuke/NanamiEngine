#pragma once
#include <map>
#include <vector>
#include <queue>

#include "Engine_Network_INetworkSystem.h"
#include "Packet/NetworkSystem_Packet.h"
#include "Object/Registry/NetworkObjectInstanceRegistry.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"
#include "operators/rx-all.hpp"

struct _ENetHost;
struct _ENetPeer;

namespace NanamiEngine::Core::Network
{
    constexpr auto PORT_ADDRESS = 1234;
    // ENet の既定タイムアウト(5s/30s)は切断検知が遅いので短くする
    constexpr enet_uint32 PEER_TIMEOUT_MIN_MS         = 1500;
    constexpr enet_uint32 PEER_TIMEOUT_MAX_MS         = 4000;
    // クライアントが自ら切断するとき、ホストへ通知が届くのを待つ上限
    constexpr enet_uint32 GRACEFUL_DISCONNECT_WAIT_MS = 300;
    // PlayerId は int8 なので 0..127 まで
    constexpr int MAX_PLAYER_ID = 127;

    class EnetUDPNetworkSystem final : public INetworkSystem
    {
    public:
        explicit EnetUDPNetworkSystem();
        ~EnetUDPNetworkSystem() override;
        void Update() override;
        void Send(const Packet& packet) override;
        void SendTo(ENetPeer* target, const Packet& packet) override;
        [[nodiscard]] std::vector<Packet> PollPackets() override;
        [[nodiscard]] INetworkObjectInstanceRegistry& GetInstanceRegistry() override;

    private:
        [[nodiscard]] PlayerId GetPlayerId() const override;
        /** ホストのみ: 離脱者の所有物を自分が引き継ぐ PlayerLeft を全員へ配り、自分の受信キューにも積む */
        void NotifyPlayerLeft(PlayerId leftId);
        void SetPlayerId(PlayerId playerId) override;
        rxcpp::observable<ENetEvent*> OnConnectPlayer() override;

    private:
        _ENetHost* host_ = nullptr;
        _ENetPeer* peer_ = nullptr;

        std::queue<Packet> receivedQueue_;
        PlayerId playerId_ = PlayerId::Invalid();

        // ホストのみ使用: 次に割り当てる PlayerId(ホスト自身が 0 を取る)と接続中 peer の一覧
        int nextPlayerId_ = 0;
        std::map<PlayerId, _ENetPeer*> peers_;

        float unreliableAccumulator_ = 0.0f;
        bool  unreliableSendAllowed_ = false;

        rxcpp::subjects::subject<ENetEvent*> onConnectPlayer_;
        NetworkObjectInstanceRegistry instanceRegistry_;
    };
}