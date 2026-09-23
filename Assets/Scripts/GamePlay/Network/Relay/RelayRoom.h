#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace GamePlay::Network
{
    /** 中継サーバーでどの部屋に入るか */
    struct RelayRoom
    {
        enum class Mode : std::uint8_t
        {
            // 同じステージの誰かと相席する(空きのある公開部屋に入る。無ければホストになる)
            Public,
            // コード付きの非公開部屋を作ってホストになる。コードは中継サーバーが決める
            Create,
            // code の非公開部屋に参加する。無ければ失敗する
            Join,
        };

        static constexpr int MODE_COUNT = 3;

        Mode        mode = Mode::Public;
        std::string code; // Join のみ

        [[nodiscard]] bool IsPrivate() const { return mode != Mode::Public; }
    };

    /** 中継サーバーから返ってきた部屋の様子。EnetRelayNetworkSystem が書き、CustomNetworkRunner 越しに読む */
    struct RelayRoomStatus
    {
        // 非公開部屋のコード。公開部屋では空
        std::string code;
        // 部屋に入る前に切れたときの理由(ロード画面に出す文言)
        std::optional<std::string> failure;
    };
}
