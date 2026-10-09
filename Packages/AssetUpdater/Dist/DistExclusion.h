#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

namespace NanamiEngine::AssetUpdater::Dist
{
    [[nodiscard]] NANAMI_API bool IsExcludedFromDistribution(const std::string& relPosix);
    
    // NOTE: ASCII の大文字だけを小文字にする
    [[nodiscard]] NANAMI_API std::string AsciiLower(std::string text);
}
