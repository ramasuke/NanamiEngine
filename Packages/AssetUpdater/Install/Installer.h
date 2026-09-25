#pragma once
#include <string>

#include "AssetUpdaterPaths.h"
#include "../Interface/IAssetUpdater.h"

namespace NanamiEngine::AssetUpdater
{
    struct ApplyResult
    {
        bool        ok = false;
        std::string error;
    };

    /** paths の Assets/ へ、一時置き場に落としたファイルを入れる */
    class Installer final
    {
    public:
        explicit Installer(AssetUpdaterPaths paths);

        /**
         * 一時置き場の照合済みファイルで Assets/ を更新する。全部成功するか、何も変わらないかのどちらか。
         */
        [[nodiscard]] ApplyResult Apply(const UpdateCheckResult& update) const;

        /** 前回の適用で消しきれなかった入れ替え用のファイルを消す */
        void RemoveLeftovers() const;

    private:
        AssetUpdaterPaths paths_;
    };
}
