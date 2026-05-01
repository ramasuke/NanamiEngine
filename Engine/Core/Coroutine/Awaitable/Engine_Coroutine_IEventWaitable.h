#pragma once
#include <coroutine>

namespace Coroutine
{
    class IEventWaitable
    {
    public:
        virtual ~IEventWaitable() = default;

        [[nodiscard]] virtual bool await_ready() const noexcept = 0;
        [[nodiscard]] virtual std::coroutine_handle<> CoroutineHandle() const = 0;
    };
}
