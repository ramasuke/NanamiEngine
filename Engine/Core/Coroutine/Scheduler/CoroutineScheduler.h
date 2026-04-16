#pragma once
#include <coroutine>
#include <vector>
#include <utility>

namespace Coroutine
{
    class IWaitForTween;
}

namespace Coroutine
{
    class IWaitForObservable;
}

namespace Coroutine
{
    struct WaitForSubscription;
}

namespace Coroutine
{
    class WaitForSeconds;

    /** @brief 登録したコルーチンに対してとUpdate()からコルーチンを進めたり管理するクラス */
    class CoroutineScheduler
    {
    public:
        void Invoke();
        void AllClear();
        
        void RegisterWaitForSeconds     (WaitForSeconds* waitForSeconds          ) { pendingWaitForSecondses_   .push_back(waitForSeconds     ); }
        void RegisterWaitForTween       (IWaitForTween * waitForTween            ) { pendingWaitForTweens_      .push_back(waitForTween       ); }
        void RegisterWaitForSubscription(WaitForSubscription* waitForSubscription) { pendingWaitForSubscription_.push_back(waitForSubscription); }
        void RegisterWaitForObservable  (IWaitForObservable * waitForObservable  ) { pendingWaitForObservables_ .push_back(waitForObservable  ); }
        /**
         * @brief コルーチンを管理実行するTaskの登録
         * 
         * @param awaited 対象コルーチン 
         * @param awaiting 待機側
         */
        void RegisterTask  (std::coroutine_handle<> awaited, std::coroutine_handle<> awaiting);
        /** @brief 非同期実行するされるコルーチンを管理実行するFutureTaskの登録 */
        void RegisterFuture(std::coroutine_handle<> awaiting) { pendingCoroutines_.emplace_back(std::noop_coroutine(), awaiting); }

    private:
        std::vector<WaitForSeconds     *> waitForSecondses_    ;
        std::vector<IWaitForTween      *> waitForTweens_       ;
        std::vector<WaitForSubscription*> waitForSubscriptions_;
        std::vector<IWaitForObservable *> waitForObservables_  ;
        std::vector<std::pair<std::coroutine_handle<>, std::coroutine_handle<>>> coroutines_;

        std::vector<WaitForSeconds     *> pendingWaitForSecondses_;
        std::vector<IWaitForTween      *> pendingWaitForTweens_;
        std::vector<WaitForSubscription*> pendingWaitForSubscription_;
        std::vector<IWaitForObservable *> pendingWaitForObservables_ ;
        std::vector<std::pair<std::coroutine_handle<>, std::coroutine_handle<>>> pendingCoroutines_;
    };
}
