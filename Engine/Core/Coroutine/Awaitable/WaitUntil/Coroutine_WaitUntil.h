#pragma once
#include <coroutine>
#include <functional>

#include "../Engine_Coroutine_ITickableWaitable.h"

namespace Coroutine
{
    class WaitUntil final : public ITickableWaitable
    {
    public:
        explicit WaitUntil(std::function<bool()> condition);
        [[nodiscard]] bool await_ready() const noexcept override;
        void await_suspend(std::coroutine_handle<> parentHandle);
        void await_resume() const noexcept {}
        void Tick(float) override;
        [[nodiscard]] std::coroutine_handle<> CoroutineHandle() const override
        {
            return parentHandle_;
        }

    private:
        std::function<bool()> condition_;
        std::coroutine_handle<> parentHandle_{};
    };
}