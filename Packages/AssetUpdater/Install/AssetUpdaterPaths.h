#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>
#include <string>

namespace NanamiEngine::AssetUpdater
{
    struct NANAMI_API AssetUpdaterPaths
    {
        std::filesystem::path gameRoot;
        std::filesystem::path installedState;
        std::filesystem::path stagingDirectory;

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
