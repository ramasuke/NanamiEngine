#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>
#include <optional>
#include <string>

namespace NanamiEngine::AssetUpdater
{
    [[nodiscard]] NANAMI_API std::optional<std::filesystem::path> ResolveAssetPath(const std::filesystem::path& gameRoot, const std::string& manifestPath);
}
