#pragma once
#include <coroutine>

namespace Coroutine
{
    class IWaitForTween
    {
    public:
        virtual ~IWaitForTween() = default;
        virtual void Tick(int deltaTime_msecs) = 0;
        [[nodiscard]] virtual std::coroutine_handle<> CoroutineHandle() const = 0;
        [[nodiscard]] virtual bool await_ready() const noexcept = 0;
    };
}
