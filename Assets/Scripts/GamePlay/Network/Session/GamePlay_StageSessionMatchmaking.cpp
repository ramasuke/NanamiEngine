#include "GamePlay_StageSessionMatchmaking.h"

#include <utility>
#include "DxLib.h"
#include "Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Core/Network/Discovery/LanSessionFinder.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../Game_CustomNetworkRunner.h"
#include "../Relay/RelayServerSettings.h"

namespace GamePlay::Network
{
    namespace
    {
        constexpr int STAGE_SESSION_SEARCH_MSECS          = 1000;
        // 同時に入った 2 人が揃ってホストにならないよう、探す長さを人ごとにずらす
        constexpr int STAGE_SESSION_SEARCH_JITTER_MSECS   = 500;
        constexpr int STAGE_SESSION_CONNECT_TIMEOUT_MSECS = 5000;

        RelayRoom s_nextStageRoom;

        /** 接続の結果が出た(runner が消えた・タイムアウトも含む)か */
        bool IsConnectAttemptSettled(const std::weak_ptr<CustomNetworkRunner>& runner, const int startedMs)
        {
            const auto locked = runner.lock();
            return !locked
                || locked->GetConnectionState() != Core::Network::ConnectionState::Connecting
                || GetNowCount() - startedMs >= STAGE_SESSION_CONNECT_TIMEOUT_MSECS;
        }
    }

    void SetNextStageRoom(RelayRoom room)
    {
        s_nextStageRoom = std::move(room);
    }

    Coroutine::Task<std::optional<std::string>> JoinOrHostStageAsync(const std::weak_ptr<CustomNetworkRunner> runner, const std::string stageKey)
    {
        const RelayRoom room = std::exchange(s_nextStageRoom, RelayRoom{});

        // 中継サーバーが使えるなら、部屋への参加もホストになるのも中継サーバーに任せる。
        // 公開部屋はつながらなければ LAN で探す。非公開部屋は中継サーバーにしか無いので、そこで諦める
        const auto relay = RelayServerSettings::Load();
        if (room.IsPrivate() && !relay.IsEnabled())
            co_return std::string("中継サーバーを使う設定になっていないので、部屋を使えません");

        if (relay.IsEnabled())
        {
            {
                const auto locked = runner.lock();
                if (!locked)
                    co_return std::nullopt;
                locked->StartRelay(stageKey, relay, room);
            }

            const int relayStartedMs = GetNowCount();
            co_await Coroutine::WaitUntil([runner, relayStartedMs] { return IsConnectAttemptSettled(runner, relayStartedMs); });

            const auto locked = runner.lock();
            if (!locked || locked->GetConnectionState() == Core::Network::ConnectionState::Connected)
                co_return std::nullopt;

            if (room.IsPrivate())
            {
                // Shutdown で理由も消えるので先に読む
                const std::string failure = locked->RelayFailure().value_or("中継サーバーにつながりませんでした");
                Module::LogWarning("StageSession: 非公開の部屋に入れませんでした: " + failure);
                locked->Shutdown();
                co_return failure;
            }

            Module::LogWarning("StageSession: 中継サーバー経由で参加できなかったので、LAN で探します");
            locked->Shutdown();
        }

        // runner はステージシーンのコンポーネント。待っている間にシーンごと消えうるので、待機をまたいで握らない
        std::optional<Core::Network::HostEndpoint> host;
        {
            Core::Network::LanSessionFinder finder(stageKey);
            const int searchMsecs = STAGE_SESSION_SEARCH_MSECS + GetRand(STAGE_SESSION_SEARCH_JITTER_MSECS);
            const int startedMs   = GetNowCount();
            while (true)
            {
                finder.Update();
                if (finder.Found() || GetNowCount() - startedMs >= searchMsecs)
                    break;
                co_await Coroutine::WaitYield();
            }
            host = finder.Found();
        }

        if (host)
        {
            {
                const auto locked = runner.lock();
                if (!locked)
                    co_return std::nullopt;
                locked->StartClient(*host);
            }

            const int connectStartedMs = GetNowCount();
            co_await Coroutine::WaitUntil([runner, connectStartedMs] { return IsConnectAttemptSettled(runner, connectStartedMs); });

            const auto locked = runner.lock();
            if (!locked || locked->GetConnectionState() == Core::Network::ConnectionState::Connected)
                co_return std::nullopt;

            // 見つけたホストが直前に抜けた・満員だった
            Module::LogWarning("StageSession: " + host->address + " に参加できなかったので、自分がホストになります");
            locked->Shutdown();
            locked->StartHost(stageKey);
            co_return std::nullopt;
        }

        if (const auto locked = runner.lock())
            locked->StartHost(stageKey);
        co_return std::nullopt;
    }
}
