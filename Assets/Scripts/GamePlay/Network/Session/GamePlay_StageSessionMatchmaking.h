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
        // NOTE: 次にステージへ入るときの部屋の入り方を決める
        void SetNextRoom(RelayRoom room);

        // NOTE: SetNextRoom で決めた部屋に入る。入れなかったときはその理由を返す
        [[nodiscard]] Coroutine::Task<std::optional<std::string>> JoinOrHostAsync(std::weak_ptr<CustomNetworkRunner> runner, std::string stageKey);

    private:
        // WARNING: コルーチンに this を持ち込まない。部屋は呼んだ時点で取り出して渡す
        static Coroutine::Task<std::optional<std::string>> JoinOrHostAsync(std::weak_ptr<CustomNetworkRunner> runner, std::string stageKey, RelayRoom room);
        [[nodiscard]] static bool IsConnectAttemptSettled(const std::weak_ptr<CustomNetworkRunner>& runner, int startedMs);

        RelayRoom nextRoom_;
    };
}
