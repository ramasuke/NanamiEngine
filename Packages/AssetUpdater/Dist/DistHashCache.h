#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace NanamiEngine::AssetUpdater::Dist
{
    class NANAMI_API DistHashCache final
    {
    public:
        using References = std::optional<std::vector<std::string>>;

        explicit DistHashCache(std::filesystem::path cachePath);

        [[nodiscard]] std::optional<std::string> HashOf(const std::filesystem::path& path, const std::string& key);
        [[nodiscard]] References RefsOf(const std::string& digest, const std::function<References()>& read);

        void Save() const;

        int hits      = 0;
        int misses    = 0;
        int refHits   = 0;
        int refMisses = 0;

    private:
        struct NANAMI_API Cached
        {
            std::int64_t  mtimeNs = 0;
            std::uint64_t size    = 0;
            std::string   sha256;
        };

        std::filesystem::path                        path_;
        std::unordered_map<std::string, Cached>      entries_;
        std::unordered_map<std::string, References>  refs_;
    };

    struct NANAMI_API DistFileStat
    {
        /** Unix 時刻のナノ秒 (Python の st_mtime_ns と同じ値なので、tools/dist 時代のキャッシュもそのまま効く) */
        std::int64_t  mtimeNs = 0;
        std::uint64_t size    = 0;
    };

    [[nodiscard]] NANAMI_API std::optional<DistFileStat> StatFile(const std::filesystem::path& path);
}
