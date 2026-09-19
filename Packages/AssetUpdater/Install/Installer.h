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

    /**
     * 一時置き場の照合済みファイルで Assets/ を更新する。全部成功するか、何も変わらないかのどちらか。
     * installed.json が無いフォルダ (開発中のリポジトリなど) には何もしない
     */
    [[nodiscard]] ApplyResult ApplyUpdate(const AssetUpdaterPaths& paths, const UpdateCheckResult& update);

    /** 前回の適用で消しきれなかった入れ替え用のファイルを消す */
    void RemoveInstallLeftovers(const AssetUpdaterPaths& paths);
}
