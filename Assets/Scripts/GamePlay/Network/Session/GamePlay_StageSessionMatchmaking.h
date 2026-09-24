#pragma once
#include <memory>
#include <optional>
#include <string>

#include "Engine/Core/Coroutine/Task/Task.h"
#include "../Relay/RelayRoom.h"

namespace GamePlay::Network
{
    class CustomNetworkRunner;

    /**
     * @brief ステージへ入るときの部屋選び。ステージ選択が次の部屋を決め、ステージのシーンがそれで入る
     * @note シーンを跨いで持つので GameCore::Game が持つ(Game::Instance().Matchmaker())
     */
    class StageMatchmaker final
    {
    public:
        /**
         * @brief 次にステージへ入るときの部屋の入り方を決める(ステージ選択から呼ぶ)
         * @note 次の JoinOrHostAsync が一度だけ使い、そのあとは公開部屋に戻る
         */
        void SetNextRoom(RelayRoom room);

        /**
         * @brief SetNextRoom で決めた部屋に入る
         *        公開部屋: 中継サーバーで同じステージの部屋に相席する。つながらなければ LAN で探し、居なければ自分がホストになる
         *        非公開部屋: 中継サーバーでコード付きの部屋を作る・コードの部屋に入る。LAN には切り替えない
         * @return 入れなかったときの理由。入れた・runner が途中で消えたときは nullopt
         * @note 入れたかどうかは runner の IsStarted() / GetConnectionState() で見る
         */
        [[nodiscard]] Coroutine::Task<std::optional<std::string>> JoinOrHostAsync(std::weak_ptr<CustomNetworkRunner> runner, std::string stageKey);

    private:
        // NOTE: 部屋は呼んだ時点で取り出して渡す。コルーチンに this を持ち込まない
        static Coroutine::Task<std::optional<std::string>> JoinOrHostAsync(std::weak_ptr<CustomNetworkRunner> runner, std::string stageKey, RelayRoom room);
        /** 接続の結果が出た(runner が消えた・タイムアウトも含む)か */
        [[nodiscard]] static bool IsConnectAttemptSettled(const std::weak_ptr<CustomNetworkRunner>& runner, int startedMs);

        RelayRoom nextRoom_;
    };
}
