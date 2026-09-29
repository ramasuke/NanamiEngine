#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

namespace NanamiEngine::AssetUpdater
{
    struct NANAMI_API DownloadResult
    {
        bool        ok        = false;
        bool        cancelled = false;
        std::string error;
    };
}
