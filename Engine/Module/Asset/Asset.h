#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Module::Asset
{
    /**
     * Asset関連をまとめたHelper
     */
    class NANAMI_API Asset final
    {
    public:
        static bool IsLoadingResource();
        static int GetLoadingResourceCount();
    };
}
