#include "GamePlay_StageSessionMatchmaking.h"

#include "DxLib.h"
#include "../../../../../Engine/Core/Coroutine/Awaitable/WaitUntil/Coroutine_WaitUntil.h"
#include "../../../../../Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "../../../../../Engine/Core/Network/Discovery/LanSessionFinder.h"
#include "../../../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../Game_CustomNetworkRunner.h"

namespace GamePlay::Network
{
    namespace
    {
        constexpr int STAGE_SESSION_SEARCH_MSECS          = 1000;
        // 同時に入った 2 人が揃ってホストにならないよう、探す長さを人ごとにずらす
        constexpr int STAGE_SESSION_SEARCH_JITTER_MSECS   = 500;
        constexpr int STAGE_SESSION_CONNECT_TIMEOUT_MSECS = 5000;
    }

    Coroutine::Task<void> JoinOrHostStageAsync(const std::weak_ptr<CustomNetworkRunner> runner, const std::string stageKey)
    {
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
                    co_return;
                locked->StartClient(*host);
            }

            const int connectStartedMs = GetNowCount();
            co_await Coroutine::WaitUntil([runner, connectStartedMs]
            {
                const auto locked = runner.lock();
                return !locked
                    || locked->GetConnectionState() != Core::Network::ConnectionState::Connecting
                    || GetNowCount() - connectStartedMs >= STAGE_SESSION_CONNECT_TIMEOUT_MSECS;
            });

            const auto locked = runner.lock();
            if (!locked || locked->GetConnectionState() == Core::Network::ConnectionState::Connected)
                co_return;

            // 見つけたホストが直前に抜けた・満員だった
            Module::LogWarning("StageSession: " + host->address + " に参加できなかったので、自分がホストになります");
            locked->Shutdown();
            locked->StartHost(stageKey);
            co_return;
        }

        if (const auto locked = runner.lock())
            locked->StartHost(stageKey);
    }
}
