#pragma once
#include <coroutine>

namespace Coroutine
{
    struct WaitForSeconds final
    {
        explicit WaitForSeconds(float duration_secs);

        [[nodiscard]] bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> parentHandle);
        void await_resume() const noexcept { }
        void Tick(float deltaTime_secs);
        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const { return parentHandle_; }

    private:
        std::coroutine_handle<> parentHandle_;
        float awaitTimeDuration_secs_;
        float awaitTimeDuring_secs_ = 0.0f;
    };
}
