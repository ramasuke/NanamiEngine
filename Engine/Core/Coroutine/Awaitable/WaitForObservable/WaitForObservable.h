#pragma once
#include <coroutine>

#include "IWaitForObservable.h"
#include "../../../Application/ApplicationBase.h"
#include "../../../Application/Window/Main/Game/GameWindow.h"
#include "../../Scheduler/CoroutineScheduler.h"
#include "../rxcpp/rx.hpp"

namespace Coroutine
{
    template<typename ArgT>
    struct WaitForObservable final : IWaitForObservable 
    {
    public:
        explicit WaitForObservable(rxcpp::observable<ArgT> observable)
            : observable_(observable)
            , isOnNexted_(false     )
        {
             compositeSubscription_ = observable.subscribe(
                rxcpp::composite_subscription(),
                [this](const ArgT& arg)
                {
                    isOnNexted_ = true;
                    parentHandle_.resume();
                });
        }
        ~WaitForObservable() override
        {
            compositeSubscription_.unsubscribe();
        }

        [[nodiscard]] bool await_ready() const noexcept override
        {
            return isOnNexted_;  
        }
        void await_suspend(const std::coroutine_handle<> parentHandle)
        {
            parentHandle_ = parentHandle;
            Core::Application::ApplicationBase::GameWindow()->LifeCycle().Coroutine()->RegisterWaitForObservable(this);
        }
        void await_resume() const noexcept { }

    private:
        rxcpp::composite_subscription compositeSubscription_;
        std::coroutine_handle<> parentHandle_;
        rxcpp::observable<ArgT> observable_;
        bool isOnNexted_; 
    };
}
