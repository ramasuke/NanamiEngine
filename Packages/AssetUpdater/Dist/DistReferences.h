#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace NanamiEngine::AssetUpdater::Dist
{
    class  DistHashCache;
    struct DistScanResult;

    constexpr auto DIST_REASON_OUTSIDE  = "Assets の外";
    constexpr auto DIST_REASON_EXCLUDED = "配信対象外";

    struct NANAMI_API DistUnshippedRef
    {
        // NOTE: 参照している側 (リポジトリルートからの '/' 区切り)
        std::string path;
        // NOTE: ファイルに書かれているとおりの参照
        std::string ref;
        std::string reason;
    };

    struct NANAMI_API DistRefReport
    {
        std::vector<DistUnshippedRef> unshipped;
        // NOTE: 参照を読み出せなかったファイル (壊れている / 知らない形式)
        std::vector<std::string>      unreadable;
        bool                          canceled = false;
    };

    // NOTE: .efkefc の INFO チャンクにある、.efkefc からの相対のアセットパス。読めなければ nullopt
    // NOTE: チャンクを解析できなければ UTF-16 テキストからパスらしいものを拾う
    [[nodiscard]] NANAMI_API std::optional<std::vector<std::string>> EfkefcAssetPaths(const std::string& bytes);

    // NOTE: .mv1 ("MV11" + dst u32 + src u32 + key u8 + DXArchive LZ) を本体に展開する。壊れていれば nullopt
    [[nodiscard]] NANAMI_API std::optional<std::string> Mv1Decode(const std::string& bytes);

    // NOTE: .mv1 が参照するテクスチャのパス (.mv1 からの相対、重複除去)
    // NOTE: 展開した本体から画像拡張子で終わる文字列を拾うだけ。元 FBX の絶対パスも混ざりうる
    [[nodiscard]] NANAMI_API std::optional<std::vector<std::string>> Mv1TexturePaths(const std::string& bytes);

    // NOTE: path (.efkefc / .mv1) の中にある参照を、書かれているとおりに返す。読めなければ nullopt
    [[nodiscard]] NANAMI_API std::optional<std::vector<std::string>> ReferencesOf(const std::filesystem::path& path);

    // NOTE: 配信する .efkefc / .mv1 が、この PC にはあるが配信されないファイルを参照していないか調べる
    [[nodiscard]] NANAMI_API DistRefReport FindUnshippedRefs(const DistScanResult& scan, const std::filesystem::path& repoRoot, const std::filesystem::path& assetsRoot,
                                                             DistHashCache& cache, const std::function<bool()>& isCanceled = {});
}
