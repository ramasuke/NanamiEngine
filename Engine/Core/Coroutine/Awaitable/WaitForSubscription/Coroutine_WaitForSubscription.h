#pragma once
#include <coroutine>

#include "../Engine_Coroutine_IEventWaitable.h"
#include "../../../../../Packages/R4/R4.h"

namespace Coroutine
{
    struct WaitForSubscription final : IEventWaitable
    {
        //NOTE: token がキャンセルされるまで待つ
        explicit WaitForSubscription(NanamiEngine::R4::CancellationToken token);

        [[nodiscard]] bool await_ready() const noexcept override;

        void await_suspend(std::coroutine_handle<> parentHandle);
        void await_resume() const noexcept {}

        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        std::coroutine_handle<> parentHandle_{};
        NanamiEngine::R4::CancellationToken token_;
        bool isReady_ = false;
    };
}