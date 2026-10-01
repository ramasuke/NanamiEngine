#include "DistScan.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <optional>
#include <string_view>

#include <windows.h>

#include "DistExclusion.h"
#include "DistHashCache.h"
#include "../Text/Utf8.h"
#include "../cereal/include/cereal/external/rapidjson/document.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    namespace
    {
        constexpr char    DIST_SCAN_UTF8_BOM[]     = "\xEF\xBB\xBF";
        constexpr wchar_t DIST_SCAN_META_SUFFIX[]  = L".meta";
        constexpr UINT    DIST_SCAN_CP932          = 932;

        using DistScanJsonValue = CEREAL_RAPIDJSON_NAMESPACE::Value;

        std::optional<std::string> DistScanFindGuid(const DistScanJsonValue& node)
        {
            if (node.IsObject())
            {
                for (auto it = node.MemberBegin(); it != node.MemberEnd(); ++it)
                {
                    if (std::string_view(it->name.GetString(), it->name.GetStringLength()) == "guid_" && it->value.IsObject())
                    {
                        const auto value = it->value.FindMember("value_");
                        if (value != it->value.MemberEnd() && value->value.IsString())
                            return std::string(value->value.GetString(), value->value.GetStringLength());
                    }
                    if (std::optional<std::string> found = DistScanFindGuid(it->value); found && !found->empty())
                        return found;
                }
            }
            else if (node.IsArray())
            {
                for (const DistScanJsonValue& item : node.GetArray())
                {
                    if (std::optional<std::string> found = DistScanFindGuid(item); found && !found->empty())
                        return found;
                }
            }
            return std::nullopt;
        }


        /** 大文字小文字を無視した、パス要素ごとの並び (tools/dist の sorted(rglob) と同じ順) */
        std::vector<std::string> DistScanSortKey(const std::filesystem::path& relative)
        {
            std::vector<std::string> key;
            for (const std::filesystem::path& part : relative)
            {
                std::wstring text = part.wstring();
                CharLowerBuffW(text.data(), static_cast<DWORD>(text.size()));
                key.push_back(WideToUtf8(text));
            }
            return key;
        }

        bool DistScanIsAscii(const std::string& text)
        {
            return std::ranges::all_of(text, [](const char c) { return static_cast<unsigned char>(c) < 0x80; });
        }
    }

    std::size_t DistScanResult::AssetCount() const
    {
        return static_cast<std::size_t>(std::ranges::count_if(entries, [](const ManifestEntry& e) { return !e.metaHash.empty(); }));
    }

    std::size_t DistScanResult::CompanionCount() const
    {
        return entries.size() - AssetCount();
    }

    std::uint64_t DistScanResult::TotalBytes() const
    {
        std::uint64_t total = 0;
        for (const ManifestEntry& entry : entries)
            total += entry.TotalSize();
        return total;
    }

    std::string DecodeUtf8OrCp932(const std::string& bytes, bool* const outWasCp932)
    {
        if (outWasCp932)
            *outWasCp932 = false;
        if (bytes.empty())
            return bytes;
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0) > 0)
            return bytes;

        if (outWasCp932)
            *outWasCp932 = true;
        const int length = MultiByteToWideChar(DIST_SCAN_CP932, 0, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
        if (length <= 0)
            return {};
        std::wstring wide(static_cast<size_t>(length), L'\0');
        MultiByteToWideChar(DIST_SCAN_CP932, 0, bytes.data(), static_cast<int>(bytes.size()), wide.data(), length);
        return WideToUtf8(wide);
    }

    DistMetaGuid ReadMetaGuid(const std::filesystem::path& metaPath)
    {
        std::ifstream stream(metaPath, std::ios::binary);
        if (!stream)
            return {};
        std::string bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        if (bytes.starts_with(DIST_SCAN_UTF8_BOM))
            bytes.erase(0, 3);

        bool wasCp932 = false;
        const std::string text = DecodeUtf8OrCp932(bytes, &wasCp932);

        CEREAL_RAPIDJSON_NAMESPACE::Document document;
        document.Parse<CEREAL_RAPIDJSON_NAMESPACE::kParseNanAndInfFlag>(text.c_str(), text.size());
        // NOTE: 壊れた .meta 1 個でビルド全体を止めない
        if (document.HasParseError())
            return {};

        DistMetaGuid result;
        result.guid     = DistScanFindGuid(document).value_or(std::string());
        result.encoding = wasCp932 ? "cp932" : "utf-8";
        return result;
    }

    std::filesystem::path Utf8ToPath(const std::string& utf8)
    {
        return std::filesystem::path(Utf8ToWide(utf8));
    }

    std::string ToRelPosix(const std::filesystem::path& path, const std::filesystem::path& repoRoot)
    {
        return WideToUtf8(path.lexically_relative(repoRoot).generic_wstring());
    }

    DistScanResult ScanAssets(const std::filesystem::path& assetsRoot, const std::filesystem::path& repoRoot, DistHashCache& cache, const std::function<bool()>& isCanceled)
    {
        DistScanResult result;
        std::error_code error;
        if (!std::filesystem::is_directory(assetsRoot, error))
        {
            result.error = "assets root がありません: " + WideToUtf8(assetsRoot.wstring());
            return result;
        }

        std::vector<std::pair<std::vector<std::string>, std::filesystem::path>> files;
        for (auto it = std::filesystem::recursive_directory_iterator(assetsRoot, error); !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error))
        {
            if (it->is_directory(error))
            {
                // ジャンクションの先は配信の対象にしない
                if (GetFileAttributesW(it->path().c_str()) & FILE_ATTRIBUTE_REPARSE_POINT)
                    it.disable_recursion_pending();
                continue;
            }
            if (it->is_regular_file(error))
                files.emplace_back(DistScanSortKey(it->path().lexically_relative(assetsRoot)), it->path());
        }
        if (error)
        {
            result.error = "Assets/ を走査できませんでした: " + error.message();
            return result;
        }
        std::ranges::sort(files, {}, &std::pair<std::vector<std::string>, std::filesystem::path>::first);

        for (const auto& [sortKey, path] : files)
        {
            if (isCanceled && isCanceled())
            {
                result.canceled = true;
                return result;
            }

            const std::string rel = ToRelPosix(path, repoRoot);
            if (AsciiLower(rel).ends_with(".meta"))
            {
                result.foldedMeta.push_back(rel);
                continue;
            }
            if (IsExcludedFromDistribution(rel))
            {
                result.excluded.push_back(rel);
                continue;
            }

            ManifestEntry entry;
            entry.path = rel;
            const std::optional<std::string> hash = cache.HashOf(path, rel);
            const std::optional<DistFileStat> stat = StatFile(path);
            if (!hash || !stat)
            {
                result.error = "読めませんでした: " + rel;
                return result;
            }
            entry.hash = *hash;
            entry.size = stat->size;

            std::filesystem::path metaPath = path;
            metaPath += DIST_SCAN_META_SUFFIX;
            if (std::filesystem::exists(metaPath, error))
            {
                const std::string metaRel = ToRelPosix(metaPath, repoRoot);
                const std::optional<std::string> metaHash = cache.HashOf(metaPath, metaRel);
                const std::optional<DistFileStat> metaStat = StatFile(metaPath);
                if (!metaHash || !metaStat)
                {
                    result.error = "読めませんでした: " + metaRel;
                    return result;
                }
                entry.metaHash = *metaHash;
                entry.metaSize = metaStat->size;

                const DistMetaGuid guid = ReadMetaGuid(metaPath);
                entry.guid = guid.guid;
                if (entry.guid.empty())
                    result.missingGuid.push_back(rel);
                if (guid.encoding == "cp932")
                    result.shiftJisMeta.push_back(metaRel);
            }

            if (!DistScanIsAscii(rel))
                result.nonAscii.push_back(rel);

            result.entries.push_back(std::move(entry));
        }
        return result;
    }
}
