#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../Engine_Coroutine_IEventWaitable.h"

namespace Coroutine
{
    struct NANAMI_API IWaitForObservable : IEventWaitable
    {
        virtual ~IWaitForObservable() = default;
    };
}
