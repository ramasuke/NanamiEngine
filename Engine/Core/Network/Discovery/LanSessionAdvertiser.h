#pragma once
#include <cstdint>
#include <string>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"

namespace NanamiEngine::Core::Network
{
    /** ホスト側: LAN から届いた探索の問い合わせに、セッションキーが一致すれば待ち受けポートを答える */
    class LanSessionAdvertiser final
    {
    public:
        LanSessionAdvertiser(std::string sessionKey, std::uint16_t sessionPort);
        ~LanSessionAdvertiser();
        LanSessionAdvertiser(const LanSessionAdvertiser&) = delete;
        LanSessionAdvertiser& operator=(const LanSessionAdvertiser&) = delete;

        /** 探索ポートを確保できたか。同じ PC で別のホストが告知していると false */
        [[nodiscard]] bool IsListening() const;
        [[nodiscard]] const std::string& SessionKey() const;
        /** 届いている問い合わせを読んで答える。毎フレーム呼ぶ */
        void Update();

    private:
        std::string   sessionKey_;
        std::uint16_t sessionPort_;
        ENetSocket    socket_ = ENET_SOCKET_NULL;
    };
}
