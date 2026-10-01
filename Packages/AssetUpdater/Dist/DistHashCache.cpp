#include "DistHashCache.h"

#include <fstream>
#include <iterator>
#include <utility>

#include <windows.h>

#include "../Hash/Sha256.h"
#include "../cereal/include/cereal/external/rapidjson/document.h"
#include "../cereal/include/cereal/external/rapidjson/stringbuffer.h"
#include "../cereal/include/cereal/external/rapidjson/writer.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    namespace
    {
        constexpr std::int64_t DIST_HASH_CACHE_UNIX_EPOCH_FILETIME = 116444736000000000LL;

        using DistJsonValue = CEREAL_RAPIDJSON_NAMESPACE::Value;
        using DistJsonWriter = CEREAL_RAPIDJSON_NAMESPACE::Writer<CEREAL_RAPIDJSON_NAMESPACE::StringBuffer>;

        void DistHashCacheWriteString(DistJsonWriter& writer, const std::string& text)
        {
            writer.String(text.c_str(), static_cast<CEREAL_RAPIDJSON_NAMESPACE::SizeType>(text.size()));
        }
    }

    std::optional<DistFileStat> StatFile(const std::filesystem::path& path)
    {
        WIN32_FILE_ATTRIBUTE_DATA data = {};
        if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data))
            return std::nullopt;

        const std::int64_t fileTime = static_cast<std::int64_t>((static_cast<std::uint64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime);
        DistFileStat stat;
        stat.mtimeNs = (fileTime - DIST_HASH_CACHE_UNIX_EPOCH_FILETIME) * 100;
        stat.size    = (static_cast<std::uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
        return stat;
    }

    DistHashCache::DistHashCache(std::filesystem::path cachePath)
        : path_(std::move(cachePath))
    {
        if (path_.empty())
            return;

        std::ifstream stream(path_, std::ios::binary);
        if (!stream)
            return;
        const std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

        CEREAL_RAPIDJSON_NAMESPACE::Document document;
        document.Parse(text.c_str(), text.size());
        if (document.HasParseError() || !document.IsObject())
            return;

        // NOTE: 参照キャッシュを足す前の形式は、パス -> [mtime, size, sha256] の平たいオブジェクト
        const DistJsonValue* hashes = &document;
        if (const auto member = document.FindMember("hashes"); member != document.MemberEnd())
        {
            hashes = member->value.IsObject() ? &member->value : nullptr;
            if (const auto refs = document.FindMember("refs"); refs != document.MemberEnd() && refs->value.IsObject())
            {
                for (auto it = refs->value.MemberBegin(); it != refs->value.MemberEnd(); ++it)
                {
                    const std::string digest(it->name.GetString(), it->name.GetStringLength());
                    if (it->value.IsNull())
                    {
                        refs_[digest] = std::nullopt;
                        continue;
                    }
                    if (!it->value.IsArray())
                        continue;
                    std::vector<std::string> list;
                    for (const DistJsonValue& item : it->value.GetArray())
                    {
                        if (item.IsString())
                            list.emplace_back(item.GetString(), item.GetStringLength());
                    }
                    refs_[digest] = std::move(list);
                }
            }
        }
        if (hashes == nullptr)
            return;

        for (auto it = hashes->MemberBegin(); it != hashes->MemberEnd(); ++it)
        {
            const DistJsonValue& value = it->value;
            if (!value.IsArray() || value.Size() < 3 || !value[0u].IsInt64() || !value[1u].IsUint64() || !value[2u].IsString())
                continue;
            Cached cached;
            cached.mtimeNs = value[0u].GetInt64();
            cached.size    = value[1u].GetUint64();
            cached.sha256.assign(value[2u].GetString(), value[2u].GetStringLength());
            entries_[std::string(it->name.GetString(), it->name.GetStringLength())] = std::move(cached);
        }
    }

    std::optional<std::string> DistHashCache::HashOf(const std::filesystem::path& path, const std::string& key)
    {
        const std::optional<DistFileStat> stat = StatFile(path);
        if (!stat)
            return std::nullopt;

        if (const auto it = entries_.find(key); it != entries_.end() && it->second.mtimeNs == stat->mtimeNs && it->second.size == stat->size)
        {
            ++hits;
            return it->second.sha256;
        }

        ++misses;
        std::optional<std::string> digest = Sha256OfFile(path);
        if (!digest)
            return std::nullopt;
        entries_[key] = Cached{ stat->mtimeNs, stat->size, *digest };
        return digest;
    }

    DistHashCache::References DistHashCache::RefsOf(const std::string& digest, const std::function<References()>& read)
    {
        if (const auto it = refs_.find(digest); it != refs_.end())
        {
            ++refHits;
            return it->second;
        }
        ++refMisses;
        References refs = read();
        refs_[digest] = refs;
        return refs;
    }

    void DistHashCache::Save() const
    {
        if (path_.empty())
            return;

        CEREAL_RAPIDJSON_NAMESPACE::StringBuffer buffer;
        DistJsonWriter writer(buffer);
        writer.StartObject();
        writer.Key("hashes");
        writer.StartObject();
        for (const auto& [key, cached] : entries_)
        {
            DistHashCacheWriteString(writer, key);
            writer.StartArray();
            writer.Int64(cached.mtimeNs);
            writer.Uint64(cached.size);
            DistHashCacheWriteString(writer, cached.sha256);
            writer.EndArray();
        }
        writer.EndObject();
        writer.Key("refs");
        writer.StartObject();
        for (const auto& [digest, refs] : refs_)
        {
            DistHashCacheWriteString(writer, digest);
            if (!refs)
            {
                writer.Null();
                continue;
            }
            writer.StartArray();
            for (const std::string& ref : *refs)
                DistHashCacheWriteString(writer, ref);
            writer.EndArray();
        }
        writer.EndObject();
        writer.EndObject();

        // NOTE: キャッシュなので書けなくても build は続ける
        std::ofstream stream(path_, std::ios::binary | std::ios::trunc);
        stream.write(buffer.GetString(), static_cast<std::streamsize>(buffer.GetSize()));
    }
}
