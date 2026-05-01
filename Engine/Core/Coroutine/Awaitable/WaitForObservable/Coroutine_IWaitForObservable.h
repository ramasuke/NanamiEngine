#pragma once
#include "../Engine_Coroutine_IEventWaitable.h"

namespace Coroutine
{
    struct IWaitForObservable : IEventWaitable
    {
        virtual ~IWaitForObservable() = default;
    };
}
