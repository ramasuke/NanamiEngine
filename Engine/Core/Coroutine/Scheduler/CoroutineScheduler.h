#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <coroutine>
#include <mutex>
#include <vector>
#include <utility>

namespace Coroutine
{
    class ITickableWaitable;
    class IEventWaitable;

    class NANAMI_API CoroutineScheduler
    {
    public:
        void Invoke();
        // 物理の固定ステップごと(OnFixedUpdate の後、OnBeginPhysics の前)に呼ばれる
        void InvokeFixed(float fixedDeltaTime);
        void AllClear();

        void RegisterTickable(ITickableWaitable* tickable) { pendingTickables_.push_back(tickable); }
        // 物理と同じ固定ステップで Tick される。物理ボディを動かす待機はこちらに登録する
        void RegisterFixedTickable(ITickableWaitable* tickable) { pendingFixedTickables_.push_back(tickable); }
        void RegisterEvent(IEventWaitable* event) { pendingEvents_.push_back(event); }
        void RegisterTask(std::coroutine_handle<> awaited, std::coroutine_handle<> awaiting);
        void RegisterFuture(std::coroutine_handle<> awaiting);

    private:
        std::vector<ITickableWaitable*> tickables_;
        std::vector<ITickableWaitable*> pendingTickables_;

        std::vector<ITickableWaitable*> fixedTickables_;
        std::vector<ITickableWaitable*> pendingFixedTickables_;

        std::vector<IEventWaitable*> events_;
        std::vector<IEventWaitable*> pendingEvents_;

        std::vector<std::pair<std::coroutine_handle<>, std::coroutine_handle<>>> coroutines_;
        std::vector<std::pair<std::coroutine_handle<>, std::coroutine_handle<>>> pendingCoroutines_;

        // RegisterFuture はワーカースレッドから呼ばれるので、ここだけ保護する
        std::vector<std::coroutine_handle<>> pendingResume_;
        std::mutex pendingResumeMutex_;
    };
}
