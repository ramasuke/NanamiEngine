#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "../Manifest/AssetManifest.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    // NOTE: サーバー上で可変なのは manifest.json だけ (manifest-<version>.json と files/<sha256> は不変)
    constexpr auto DIST_BLOB_CACHE_CONTROL     = "Cache-Control: public, max-age=31536000, immutable";
    constexpr auto DIST_MANIFEST_CACHE_CONTROL = "Cache-Control: no-cache";

    struct NANAMI_API DistBlob
    {
        std::string           hash;
        std::filesystem::path source;
        std::uint64_t         size = 0;
    };

    // NOTE: マニフェストが参照する全ブロブ (本体 + .meta)。中身が同じものは 1 つにまとまる。キーはハッシュ
    [[nodiscard]] NANAMI_API std::map<std::string, DistBlob> BlobsOf(const AssetManifest& manifest, const std::filesystem::path& repoRoot);

    // NOTE: remote に無いものだけ (ハッシュ順)
    [[nodiscard]] NANAMI_API std::vector<DistBlob> PlanUpload(const std::map<std::string, DistBlob>& blobs, const std::unordered_set<std::string>& existing);

    // NOTE: stagingDir/<hash> へコピーしながらハッシュを取り直し、食い違ったものを返す
    // NOTE: stagingDir が空ならコピーせず検証だけする
    [[nodiscard]] NANAMI_API std::vector<std::string> StageBlobs(const std::vector<DistBlob>& blobs, const std::filesystem::path& stagingDir,
                                                                 const std::function<bool()>& isCanceled = {});

    // NOTE: 公開中の版にある、実行中は差し替えられないファイル (フォント) のうち、変更・削除されるもの
    [[nodiscard]] NANAMI_API std::vector<std::string> LockedFileChanges(const AssetManifest& live, const AssetManifest& next);
}
