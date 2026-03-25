#pragma once
#include <coroutine>

#include "../rxcpp/rx.hpp"

namespace Coroutine
{
    struct WaitForSubscription final 
    {
        explicit WaitForSubscription(rxcpp::composite_subscription subscription);

        [[nodiscard]] bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> parentHandle);
        void await_resume() const noexcept { }
        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const { return parentHandle_; }

    private:
        std::coroutine_handle<> parentHandle_;
        const rxcpp::composite_subscription subscription_;
        bool isReady_ = false;
    };
}
