#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <functional>
#include <string>

namespace NanamiEngine::Module::StaticReflection
{
    // NOTE: DrawCategoryMenu の 1 項目。category は "A::B" で入れ子になり、空ならトップレベルに並ぶ
    struct NANAMI_API CategoryMenuItem
    {
        std::string           category;
        std::string           label;
        bool                  enabled;
        std::function<void()> onSelect;
    };
}
