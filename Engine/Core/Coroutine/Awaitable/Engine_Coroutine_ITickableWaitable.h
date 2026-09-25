#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <coroutine>

namespace Coroutine
{
    class NANAMI_API ITickableWaitable
    {
    public:
        virtual ~ITickableWaitable() = default;

        virtual void Tick(float dt) = 0;
        [[nodiscard]] virtual bool await_ready() const noexcept = 0;
        [[nodiscard]] virtual std::coroutine_handle<> CoroutineHandle() const = 0;
    };
}
