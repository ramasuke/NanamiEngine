#pragma once
#include <coroutine>

#include "../Engine_Coroutine_ITickableWaitable.h"

namespace Coroutine
{
    struct WaitForSeconds final : ITickableWaitable
    {
        explicit WaitForSeconds(float duration_secs);

        [[nodiscard]] bool await_ready() const noexcept override;
        void await_suspend(std::coroutine_handle<> parentHandle);
        void await_resume() const noexcept {}

        void Tick(float deltaTime_secs) override;

        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        std::coroutine_handle<> parentHandle_{};
        float awaitTimeDuration_secs_;
        float awaitTimeDuring_secs_ = 0.0f;
    };
}
