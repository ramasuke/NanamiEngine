#pragma once
#include <memory>
#include <optional>
#include <string>

#include "Engine/Core/Coroutine/Task/Task.h"
#include "../Relay/RelayRoom.h"

namespace GamePlay::Network
{
    class CustomNetworkRunner;

    class StageMatchmaker final
    {
    public:
        /**
         * @brief 次にステージへ入るときの部屋の入り方を決める
         */
        void SetNextRoom(RelayRoom room);

        /**
         * @brief SetNextRoom で決めた部屋に入る。
         * * @return 入れなかったときの理由
         */
        [[nodiscard]] Coroutine::Task<std::optional<std::string>> JoinOrHostAsync(std::weak_ptr<CustomNetworkRunner> runner, std::string stageKey);

    private:
        // NOTE: 部屋は呼んだ時点で取り出して渡す。コルーチンに this を持ち込まない
        static Coroutine::Task<std::optional<std::string>> JoinOrHostAsync(std::weak_ptr<CustomNetworkRunner> runner, std::string stageKey, RelayRoom room);
        /** 接続の結果が出たか */
        [[nodiscard]] static bool IsConnectAttemptSettled(const std::weak_ptr<CustomNetworkRunner>& runner, int startedMs);

        RelayRoom nextRoom_;
    };
}
