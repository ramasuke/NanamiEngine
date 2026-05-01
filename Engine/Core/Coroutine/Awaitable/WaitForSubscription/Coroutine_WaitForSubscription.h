#pragma once
#include <coroutine>

#include "../Engine_Coroutine_IEventWaitable.h"
#include "../rxcpp/rx.hpp"

namespace Coroutine
{
    struct WaitForSubscription final : IEventWaitable
    {
        explicit WaitForSubscription(rxcpp::composite_subscription subscription);

        [[nodiscard]] bool await_ready() const noexcept override;

        void await_suspend(std::coroutine_handle<> parentHandle);
        void await_resume() const noexcept {}

        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        std::coroutine_handle<> parentHandle_{};
        rxcpp::composite_subscription subscription_;
        bool isReady_ = false;
    };
}