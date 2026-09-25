#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>
#include <optional>
#include <string>

namespace NanamiEngine::AssetUpdater
{
    /**
     * マニフェストの path (UTF-8、"Assets/..." 形式) を gameRoot 下の実パスにする。
     * Assets/ の外を指しうるものは nullopt。書き込みと削除の両方で必ずこれを通す
     */
    [[nodiscard]] NANAMI_API std::optional<std::filesystem::path> ResolveAssetPath(const std::filesystem::path& gameRoot, const std::string& manifestPath);
}
