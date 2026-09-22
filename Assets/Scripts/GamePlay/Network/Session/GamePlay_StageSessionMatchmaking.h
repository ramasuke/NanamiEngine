#pragma once
#include <memory>
#include <string>

#include "Engine/Core/Coroutine/Task/Task.h"

namespace GamePlay::Network
{
    class CustomNetworkRunner;

    /**
     * @brief 同じステージのホストを LAN で探し、居れば参加・居なければ自分がホストになる
     * @note 結果は runner の IsStarted() / GetConnectionState() で見る。runner が途中で消えたら何もせずに戻る
     */
    Coroutine::Task<void> JoinOrHostStageAsync(std::weak_ptr<CustomNetworkRunner> runner, std::string stageKey);
}
