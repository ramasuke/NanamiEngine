#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>

namespace NanamiEngine::AssetUpdater
{
    struct NANAMI_API InstalledStateResult
    {
        bool        ok       = false;
        bool        canceled = false;
        std::string error;
        std::size_t entryCount = 0;
    };

    /**
     * gameRoot/Assets/ の中身を実際にハッシュして、それを表す installed.json を書く。
     * 書き出したゲームは、これで配信中の manifest.json との差分だけを更新する
     */
    class NANAMI_API InstalledStateWriter final
    {
    public:
        InstalledStateWriter(std::filesystem::path gameRoot, std::filesystem::path installedState);

        [[nodiscard]] InstalledStateResult Write(const std::function<bool()>& isCanceled) const;

    private:
        std::filesystem::path gameRoot_;
        std::filesystem::path installedState_;
    };
}
