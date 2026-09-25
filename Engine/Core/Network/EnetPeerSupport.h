#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "enet/types.h"

struct _ENetHost;
struct _ENetPeer;

namespace NanamiEngine::Core::Network
{
    // ENet の既定タイムアウト(5s/30s)は切断検知が遅いので短くする
    constexpr enet_uint32 PEER_TIMEOUT_MIN_MS         = 1500;
    constexpr enet_uint32 PEER_TIMEOUT_MAX_MS         = 4000;
    // クライアントが自ら切断するとき、相手へ通知が届くのを待つ上限
    constexpr enet_uint32 GRACEFUL_DISCONNECT_WAIT_MS = 300;
    // PlayerId は int8 なので 0..127 まで
    constexpr int MAX_PLAYER_ID = 127;

    /** 届かなくてよいパケットを UnreliableSendRate(Hz) まで間引く。毎フレーム Tick() する */
    class NANAMI_API UnreliableSendThrottle final
    {
    public:
        void Tick();
        [[nodiscard]] bool IsSendAllowed() const { return sendAllowed_; }

    private:
        float accumulator_ = 0.0f;
        bool  sendAllowed_ = false;
    };

    /** 接続中の peer へ切断を通知し、届くまで GRACEFUL_DISCONNECT_WAIT_MS だけ host を回す */
    NANAMI_API void DisconnectGracefully(_ENetHost* host, _ENetPeer* peer);
}
