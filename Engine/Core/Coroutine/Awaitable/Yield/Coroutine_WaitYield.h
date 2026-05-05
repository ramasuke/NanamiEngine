#pragma once
#include <coroutine>

#include "../Engine_Coroutine_ITickableWaitable.h"

namespace Coroutine
{
    class WaitYield final : public ITickableWaitable
    {
    public:
        WaitYield() = default;

        [[nodiscard]] bool await_ready() const noexcept override
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> parentHandle);
        void await_resume() const noexcept {}

        void Tick(float) override;

        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

        [[nodiscard]] bool IsCompleted() const
        {
            return completed_;
        }

    private:
        std::coroutine_handle<> parentHandle_{};
        bool completed_ = false;
        bool firstTick_ = true;
    };
}