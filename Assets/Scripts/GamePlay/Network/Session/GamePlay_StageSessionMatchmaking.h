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
     * @brief 次にステージへ入るときの部屋の入り方を決める(ステージ選択から呼ぶ)
     * @note 次の JoinOrHostStageAsync が一度だけ使い、そのあとは公開部屋に戻る
     */
    void SetNextStageRoom(RelayRoom room);

    /**
     * @brief SetNextStageRoom で決めた部屋に入る
     *        公開部屋: 中継サーバーで同じステージの部屋に相席する。つながらなければ LAN で探し、居なければ自分がホストになる
     *        非公開部屋: 中継サーバーでコード付きの部屋を作る・コードの部屋に入る。LAN には切り替えない
     * @return 入れなかったときの理由。入れた・runner が途中で消えたときは nullopt
     * @note 入れたかどうかは runner の IsStarted() / GetConnectionState() で見る
     */
    Coroutine::Task<std::optional<std::string>> JoinOrHostStageAsync(std::weak_ptr<CustomNetworkRunner> runner, std::string stageKey);
}
