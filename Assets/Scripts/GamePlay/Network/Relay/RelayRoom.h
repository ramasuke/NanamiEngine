#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace GamePlay::Network
{
    // NOTE: 中継サーバーでどの部屋に入るか
    struct RelayRoom
    {
        enum class Mode : std::uint8_t
        {
            // NOTE: 同じステージの誰かと相席する
            Public,
            // NOTE: コード付きの非公開部屋を作ってホストになる。コードは中継サーバーが決める
            Create,
            // NOTE: code の非公開部屋に参加する。無ければ失敗する
            Join,
        };

        static constexpr int MODE_COUNT = 3;

        Mode        mode = Mode::Public;
        std::string code; // Join のみ

        [[nodiscard]] bool IsPrivate() const { return mode != Mode::Public; }
    };

    // NOTE: 中継サーバーから返ってきた部屋の状態
    struct RelayRoomStatus
    {
        // NOTE: 非公開部屋のコード。公開部屋では空
        std::string code;
        std::optional<std::string> failure;
    };
}
