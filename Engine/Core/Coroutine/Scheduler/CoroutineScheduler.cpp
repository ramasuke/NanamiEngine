#include "CoroutineScheduler.h"

#include <unordered_set>
#include "../../Application/Time/Time.h"
#include "../Awaitable/Engine_Coroutine_ITickableWaitable.h"
#include "../Awaitable/Engine_Coroutine_IEventWaitable.h"

namespace Coroutine
{
    void CoroutineScheduler::RegisterTask(
        std::coroutine_handle<> awaited,
        std::coroutine_handle<> awaiting)
    {
        pendingCoroutines_.emplace_back(awaited, awaiting);
        awaited.resume();
    }

    void CoroutineScheduler::RegisterFuture(
        const std::coroutine_handle<> awaiting)
    {
        pendingResume_.push_back(awaiting);
    }

    void CoroutineScheduler::Invoke()
    {
        const float deltaTime = NanamiEngine::Time::DeltaTime();

        // Tickable
        for (auto it = tickables_.begin(); it != tickables_.end();)
        {
            auto* waitable = *it;
            waitable->Tick(deltaTime);

            if (waitable->await_ready())
            {
                waitable->CoroutineHandle().resume();
                it = tickables_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // Event
        for (auto it = events_.begin(); it != events_.end();)
        {
            if (const auto* waitable = *it; waitable->await_ready())
            {
                waitable->CoroutineHandle().resume();
                it = events_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // Coroutine chain
        for (auto it = coroutines_.begin(); it != coroutines_.end();)
        {
            if (auto& [awaited, awaiting] = *it; awaited.done())
            {
                auto awaitingHandle = awaiting;
                it = coroutines_.erase(it);
                awaitingHandle.resume();
            }
            else
            {
                ++it;
            }
        }

        // pending反映
        if (!pendingTickables_.empty())
        {
            tickables_.insert(tickables_.end(),
                pendingTickables_.begin(),
                pendingTickables_.end());
            pendingTickables_.clear();
        }

        if (!pendingEvents_.empty())
        {
            events_.insert(events_.end(),
                pendingEvents_.begin(),
                pendingEvents_.end());
            pendingEvents_.clear();
        }

        if (!pendingCoroutines_.empty())
        {
            coroutines_.insert(coroutines_.end(),
                pendingCoroutines_.begin(),
                pendingCoroutines_.end());
            pendingCoroutines_.clear();
        }

        // Future resume
        for (auto coroutineHandle : pendingResume_)
        {
            coroutineHandle.resume();
        }
        pendingResume_.clear();
    }

    void CoroutineScheduler::AllClear()
    {
        std::unordered_set<void*> destroyed;

        auto destroyIfNeeded = [&](const std::coroutine_handle<> h)
        {
            if (!h) return;

            void* addr = h.address();
            if (destroyed.contains(addr)) return;

            if (!h.done())
                h.destroy();

            destroyed.insert(addr);
        };

        // Tickable
        for (const auto* waitable : tickables_)
            destroyIfNeeded(waitable->CoroutineHandle());
        for (const auto* waitable : pendingTickables_)
            destroyIfNeeded(waitable->CoroutineHandle());

        // Event
        for (const auto* waitable : events_)
            destroyIfNeeded(waitable->CoroutineHandle());
        for (const auto* waitable : pendingEvents_)
            destroyIfNeeded(waitable->CoroutineHandle());

        // Coroutine
        for (auto& [awaited, awaiting] : coroutines_)
        {
            destroyIfNeeded(awaited);
            destroyIfNeeded(awaiting);
        }
        for (auto& [awaited, awaiting] : pendingCoroutines_)
        {
            destroyIfNeeded(awaited);
            destroyIfNeeded(awaiting);
        }

        tickables_        .clear();
        pendingTickables_ .clear();
        events_           .clear();
        pendingEvents_    .clear();
        coroutines_       .clear();
        pendingCoroutines_.clear();
        pendingResume_    .clear();
    }
}