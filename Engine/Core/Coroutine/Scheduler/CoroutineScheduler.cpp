#include "CoroutineScheduler.h"

#include <unordered_set>

#include "../../Application/Time/Time.h"
#include "../Awaitable/WaitForObservable/IWaitForObservable.h"
#include "../Awaitable/WaitForSeconds/WaitForSeconds.h"
#include "../Awaitable/WaitForSubscription/WaitForSubscription.h"
#include "../Awaitable/WaitForTween/IWaitForTween.h"

namespace Coroutine
{
    void CoroutineScheduler::RegisterTask(std::coroutine_handle<> awaited, std::coroutine_handle<> awaiting)
    {
        pendingCoroutines_.emplace_back(awaited, awaiting);
        awaited.resume();
    }
    
    void CoroutineScheduler::Invoke()
    {
        for (auto it = waitForSecondses_.begin(); it != waitForSecondses_.end();)
        {
            WaitForSeconds* waitable = *it;
            waitable->Tick(NanamiEngine::Time::DeltaTime());
            if (waitable->await_ready())
            {
                waitable->CoroutineHandle().resume();
                it = waitForSecondses_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    
        for (auto it = waitForTweens_.begin(); it != waitForTweens_.end();)
        {
            IWaitForTween* waitableTween = *it;
            waitableTween->Tick(NanamiEngine::Time::DeltaTime() * 1000.0f);
            if (waitableTween->await_ready())
            {
                it = waitForTweens_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for (auto it = waitForSubscriptions_.begin(); it != waitForSubscriptions_.end();)
        {
            WaitForSubscription* waitForSubscription = *it;
            if (waitForSubscription->await_ready())
            {
                it = waitForSubscriptions_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for (auto it = waitForObservables_.begin(); it != waitForObservables_.end();)
        {
            if (const IWaitForObservable* waitForObservable = *it; waitForObservable->await_ready())
            {
                it = waitForObservables_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    
        // --- コルーチンの進行 ---
        for (auto it = coroutines_.begin(); it != coroutines_.end();) {
            if (auto& [awaited, awaiting] = *it; awaited.done()) {
                auto awaitingHandle = awaiting;
                it = coroutines_.erase(it);
                awaitingHandle.resume();
            } else {
                ++it;
            }
        }
    
        if (!pendingWaitForSecondses_.empty())
        {
            waitForSecondses_.insert(waitForSecondses_.end(),
                                     pendingWaitForSecondses_.begin(),
                                     pendingWaitForSecondses_.end());
            pendingWaitForSecondses_.clear();
        }
    
        if (!pendingWaitForTweens_.empty())
        {
            waitForTweens_.insert(waitForTweens_.end(),
                                  pendingWaitForTweens_.begin(),
                                  pendingWaitForTweens_.end());
            pendingWaitForTweens_.clear();
        }
    
        if (!pendingCoroutines_.empty())
        {
            coroutines_.insert(coroutines_.end(),
                               pendingCoroutines_.begin(),
                               pendingCoroutines_.end());
            pendingCoroutines_.clear();
        }

        if (!pendingWaitForSubscription_.empty())
        {
            waitForSubscriptions_.insert(waitForSubscriptions_.end(),
                               pendingWaitForSubscription_.begin(),
                               pendingWaitForSubscription_.end());
            pendingWaitForSubscription_.clear();
        }
        
        if (!pendingWaitForObservables_.empty())
        {
            waitForObservables_.insert(waitForObservables_.end(),
                               pendingWaitForObservables_.begin(),
                               pendingWaitForObservables_.end());
            pendingWaitForObservables_.clear();
        }
    }
    
    void CoroutineScheduler::AllClear()
    {
        // すでに destroy したコルーチンフレームを記録
        std::unordered_set<void*> destroyed;

        auto destroyIfNeeded = [&](std::coroutine_handle<> h)
        {
            if (!h) return;

            void* addr = h.address();
            if (destroyed.contains(addr)) return; // 二重破棄防止

            if (!h.done())
                h.destroy();

            destroyed.insert(addr);
        };

        for (const WaitForSeconds* waitForSeconds : waitForSecondses_)
        {
            destroyIfNeeded(waitForSeconds->CoroutineHandle());
        }
        for (const WaitForSeconds* waitForSeconds : pendingWaitForSecondses_)
        {
            destroyIfNeeded(waitForSeconds->CoroutineHandle());
        }

        for (const IWaitForTween* tween : waitForTweens_)
        {
            destroyIfNeeded(tween->CoroutineHandle());
        }
        for (const IWaitForTween* tween : pendingWaitForTweens_)
        {
            destroyIfNeeded(tween->CoroutineHandle());
        }

        // ---- destroy pending coroutines ----
        for (auto& [awaited, awaiting] : pendingCoroutines_)
        {
            destroyIfNeeded(awaited);
            destroyIfNeeded(awaiting);
        }

        // ---- destroy active coroutines ----
        for (auto& [awaited, awaiting] : coroutines_)
        {
            destroyIfNeeded(awaited);
            destroyIfNeeded(awaiting);
        }

        // ---- コンテナを完全クリア ----
        waitForSecondses_       .clear();
        pendingWaitForSecondses_.clear();
        waitForTweens_          .clear();
        pendingWaitForTweens_   .clear();
        coroutines_             .clear();
        pendingCoroutines_      .clear();
    }
}
