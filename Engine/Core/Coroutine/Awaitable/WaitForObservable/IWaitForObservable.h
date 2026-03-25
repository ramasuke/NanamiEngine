#pragma once

namespace Coroutine
{
    struct IWaitForObservable
    {
    public:
        virtual ~IWaitForObservable() = default;
        [[nodiscard]] virtual bool await_ready() const noexcept = 0;
    };
}
