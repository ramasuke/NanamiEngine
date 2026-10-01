#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

namespace NanamiEngine::AssetUpdater
{
    [[nodiscard]] NANAMI_API bool IsValidVersionString(const std::string& version);

    [[nodiscard]] NANAMI_API bool IsOlderVersion(const std::string& left, const std::string& right);
}
