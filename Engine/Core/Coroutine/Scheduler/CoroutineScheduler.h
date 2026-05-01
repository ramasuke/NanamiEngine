#pragma once
#include <coroutine>
#include <vector>
#include <utility>

namespace Coroutine
{
    class ITickableWaitable;
    class IEventWaitable;

    class CoroutineScheduler
    {
    public:
        void Invoke();
        void AllClear();
        
        void RegisterTickable(ITickableWaitable* tickable) { pendingTickables_.push_back(tickable); }
        void RegisterEvent(IEventWaitable* event) { pendingEvents_.push_back(event); }
        void RegisterTask(std::coroutine_handle<> awaited, std::coroutine_handle<> awaiting);
        void RegisterFuture(std::coroutine_handle<> awaiting);

    private:
        std::vector<ITickableWaitable*> tickables_;
        std::vector<ITickableWaitable*> pendingTickables_;
        
        std::vector<IEventWaitable*> events_;
        std::vector<IEventWaitable*> pendingEvents_;

        std::vector<std::pair<std::coroutine_handle<>, std::coroutine_handle<>>> coroutines_;
        std::vector<std::pair<std::coroutine_handle<>, std::coroutine_handle<>>> pendingCoroutines_;

        std::vector<std::coroutine_handle<>> pendingResume_;
    };
}
