#pragma once
#include <cstdint>
#include <optional>
#include <string>

#include "../Packet/ByteBuffer/Packet_ByteBuffer.h"

namespace NanamiEngine::Core::Network
{
    /** LAN でセッションを探す問い合わせ/返事を受けるポート(ゲームの待ち受けポートの隣) */
    constexpr std::uint16_t LAN_DISCOVERY_PORT = 1235;
    /** 1 通の上限。セッションキーはこれに収まる長さにする */
    constexpr size_t LAN_DISCOVERY_MAX_MESSAGE_SIZE = 512;

    struct LanSessionQuery
    {
        std::string sessionKey;
    };

    struct LanSessionReply
    {
        std::string   sessionKey;
        std::uint16_t port = 0;
    };

    /** LAN セッション探索の問い合わせ/返事と UDP で送るバイト列の変換。不正なデータは nullopt */
    class LanSessionMessage final
    {
    public:
        [[nodiscard]] static ByteBuffer EncodeQuery(const LanSessionQuery& query);
        [[nodiscard]] static ByteBuffer EncodeReply(const LanSessionReply& reply);
        [[nodiscard]] static std::optional<LanSessionQuery> DecodeQuery(const uint8_t* data, size_t size);
        [[nodiscard]] static std::optional<LanSessionReply> DecodeReply(const uint8_t* data, size_t size);
    };
}
