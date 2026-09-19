#pragma once
#include <filesystem>
#include <string>

namespace NanamiEngine::AssetUpdater
{
    struct AssetUpdaterPaths
    {
        /** Assets/ があるフォルダ。エンジンの作業ディレクトリと同じ */
        std::filesystem::path gameRoot;
        std::filesystem::path installedState;
        std::filesystem::path stagingDirectory;
    };

    [[nodiscard]] inline std::filesystem::path StagedBlobPath(const AssetUpdaterPaths& paths, const std::string& hash)
    {
        return paths.stagingDirectory / "files" / hash;
    }
}
