#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <optional>
#include <string>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"
#include "../Mode/NetworkSystem_HostEndpoint.h"

namespace NanamiEngine::Core::Network
{
    // NOTE: セッションキーが一致するホストを LAN から探す (同一 PC と各サブネットへブロードキャスト)
    class NANAMI_API LanSessionFinder final
    {
    public:
        explicit LanSessionFinder(std::string sessionKey);
        ~LanSessionFinder();
        LanSessionFinder(const LanSessionFinder&) = delete;
        LanSessionFinder& operator=(const LanSessionFinder&) = delete;

        // NOTE: 一定間隔で問い合わせを送り直し、届いた返事を読む。毎フレーム呼ぶ
        void Update();
        // NOTE: 最初に答えたホスト
        [[nodiscard]] const std::optional<HostEndpoint>& Found() const;

    private:
        void SendQuery() const;
        void ReceiveReplies();

        std::string              sessionKey_;
        ENetSocket               socket_ = ENET_SOCKET_NULL;
        std::vector<ENetAddress> queryTargets_;
        std::optional<enet_uint32> lastQueryTime_;
        std::optional<HostEndpoint> found_;
    };
}
