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

        /** ダウンロードしたファイルを中身のハッシュ名で置くフォルダ */
        [[nodiscard]] std::filesystem::path StagedBlobDirectory() const
        {
            return stagingDirectory / "files";
        }

        [[nodiscard]] std::filesystem::path StagedBlobPath(const std::string& hash) const
        {
            return StagedBlobDirectory() / hash;
        }
    };
}
