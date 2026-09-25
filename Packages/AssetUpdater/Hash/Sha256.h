#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>
#include <optional>
#include <string>

namespace NanamiEngine::AssetUpdater
{
    /** 小文字16進の SHA-256。読めなければ nullopt */
    [[nodiscard]] NANAMI_API std::optional<std::string> Sha256OfFile(const std::filesystem::path& filePath);
}
