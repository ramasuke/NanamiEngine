#pragma once
#include <filesystem>
#include <optional>
#include <string>

namespace NanamiEngine::AssetUpdater
{
    /** 小文字16進の SHA-256。読めなければ nullopt */
    [[nodiscard]] std::optional<std::string> Sha256OfFile(const std::filesystem::path& filePath);
}
