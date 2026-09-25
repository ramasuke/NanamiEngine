#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::R4
{
    ///NOTE: 値なしの通知用構造体（R3 の Unit）
    struct NANAMI_API Unit final
    {
        friend constexpr bool operator==(Unit, Unit) { return true; }
    };
}
