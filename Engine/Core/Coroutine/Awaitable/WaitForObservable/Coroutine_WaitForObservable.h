#pragma once
#include <coroutine>

#include "Coroutine_IWaitForObservable.h"
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
        {
            compositeSubscription_ = observable_.subscribe(
                rxcpp::composite_subscription(),
                [this](const ArgT&)
                {
                    isReady_ = true;
                });
        }

        ~WaitForObservable() override
        {
            compositeSubscription_.unsubscribe();
        }

        [[nodiscard]] bool await_ready() const noexcept override
        {
            return isReady_;
        }

        void await_suspend(std::coroutine_handle<> parentHandle)
        {
            parentHandle_ = parentHandle;

            Core::Application::ApplicationBase::GameWindow()
                ->LifeCycle().Coroutine()
                ->RegisterEvent(this);
        }

        void await_resume() const noexcept {}

        std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        rxcpp::composite_subscription compositeSubscription_;
        std::coroutine_handle<> parentHandle_;
        rxcpp::observable<ArgT> observable_;
        bool isReady_ = false;
    };
}