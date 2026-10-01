#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>
#include <optional>
#include <string>

namespace NanamiEngine::AssetUpdater
{
    /** 小文字16進の SHA-256。読めなければ nullopt */
    [[nodiscard]] NANAMI_API std::optional<std::string> Sha256OfFile(const std::filesystem::path& filePath);

    /** 読んだバイト列を copyTo へ書きながら取る Sha256OfFile。copyTo が空なら書かない。書けなかったときも nullopt */
    [[nodiscard]] NANAMI_API std::optional<std::string> Sha256OfFileCopyingTo(const std::filesystem::path& filePath, const std::filesystem::path& copyTo);
}
