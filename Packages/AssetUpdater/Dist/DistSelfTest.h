#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>
#include <functional>
#include <string>

namespace NanamiEngine::AssetUpdater::Dist
{
    [[nodiscard]] NANAMI_API int RunDistSelfTest(const std::function<void(std::string)>& print, const std::filesystem::path& repoRoot);
}
