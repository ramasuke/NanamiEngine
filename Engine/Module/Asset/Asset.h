#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Module::Asset
{
    // NOTE: アセットの読み込み状況の問い合わせ
    class NANAMI_API Asset final
    {
    public:
        static bool IsLoadingResource();
        static int GetLoadingResourceCount();
    };
}
