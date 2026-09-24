#include "AssetManifest.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "../cereal/include/cereal/external/rapidjson/document.h"

namespace NanamiEngine::AssetUpdater
{
    namespace
    {
        bool ManifestReadString(const CEREAL_RAPIDJSON_NAMESPACE::Value& object, const char* key, std::string& outText)
        {
            const auto member = object.FindMember(key);
            if (member == object.MemberEnd() || !member->value.IsString())
                return false;

            outText.assign(member->value.GetString(), member->value.GetStringLength());
            return true;
        }

        std::uint64_t ManifestReadUInt64(const CEREAL_RAPIDJSON_NAMESPACE::Value& object, const char* key)
        {
            const auto member = object.FindMember(key);
            if (member == object.MemberEnd() || !member->value.IsUint64())
                return 0;

            return member->value.GetUint64();
        }

        int ManifestReadInt(const CEREAL_RAPIDJSON_NAMESPACE::Value& object, const char* key)
        {
            const auto member = object.FindMember(key);
            if (member == object.MemberEnd() || !member->value.IsInt())
                return 0;

            return member->value.GetInt();
        }
    }

    bool AssetManifest::TryParse(const std::string& json, AssetManifest& outManifest, std::string& outError)
    {
        CEREAL_RAPIDJSON_NAMESPACE::Document document;
        document.Parse(json.c_str(), json.size());

        if (document.HasParseError())
        {
            outError = "JSON を解析できません (offset " + std::to_string(document.GetErrorOffset()) + ")";
            return false;
        }
        if (!document.IsObject())
        {
            outError = "ルートがオブジェクトではありません";
            return false;
        }

        AssetManifest parsed;
        parsed.schema = ManifestReadInt(document, "schema");
        ManifestReadString(document, "version",               parsed.version);
        ManifestReadString(document, "requiredClientVersion", parsed.requiredClientVersion);
        ManifestReadString(document, "baseUrl",               parsed.baseUrl);

        const auto entriesMember = document.FindMember("entries");
        if (entriesMember == document.MemberEnd() || !entriesMember->value.IsArray())
        {
            outError = "entries 配列がありません";
            return false;
        }

        const auto& entriesValue = entriesMember->value;
        parsed.entries.reserve(entriesValue.Size());
        for (CEREAL_RAPIDJSON_NAMESPACE::SizeType i = 0; i < entriesValue.Size(); ++i)
        {
            const auto& entryValue = entriesValue[i];
            if (!entryValue.IsObject())
            {
                outError = "entries[" + std::to_string(i) + "] がオブジェクトではありません";
                return false;
            }

            ManifestEntry entry;
            if (!ManifestReadString(entryValue, "path", entry.path))
            {
                outError = "entries[" + std::to_string(i) + "] に path がありません";
                return false;
            }
            if (!ManifestReadString(entryValue, "hash", entry.hash))
            {
                outError = "entries[" + std::to_string(i) + "] に hash がありません";
                return false;
            }
            ManifestReadString(entryValue, "guid",     entry.guid);
            ManifestReadString(entryValue, "metaHash", entry.metaHash);
            entry.size     = ManifestReadUInt64(entryValue, "size");
            entry.metaSize = ManifestReadUInt64(entryValue, "metaSize");

            parsed.entries.push_back(std::move(entry));
        }

        outManifest = std::move(parsed);
        return true;
    }

    bool AssetManifest::TryLoadFile(const std::filesystem::path& filePath, AssetManifest& outManifest, std::string& outError)
    {
        std::ifstream stream(filePath, std::ios::binary);
        if (!stream)
        {
            outError = "ファイルを開けません: " + filePath.string();
            return false;
        }

        std::ostringstream buffer;
        buffer << stream.rdbuf();
        return TryParse(buffer.str(), outManifest, outError);
    }

    bool ManifestDiff::IsUpToDate() const
    {
        return added.empty() && changed.empty() && removedPaths.empty();
    }

    std::size_t ManifestDiff::UpdateCount() const
    {
        return added.size() + changed.size();
    }

    ManifestDiff ManifestDiff::Between(const AssetManifest& installed, const AssetManifest& remote)
    {
        std::unordered_map<std::string, const ManifestEntry*> installedByPath;
        installedByPath.reserve(installed.entries.size());
        for (const auto& entry : installed.entries)
        {
            installedByPath.emplace(entry.path, &entry);
        }

        ManifestDiff diff;
        for (const auto& entry : remote.entries)
        {
            const auto found = installedByPath.find(entry.path);
            if (found == installedByPath.end())
            {
                diff.downloadBytes += entry.TotalSize();
                diff.added.push_back(entry);
                continue;
            }

            if (found->second->hash != entry.hash || found->second->metaHash != entry.metaHash)
            {
                diff.downloadBytes += entry.TotalSize();
                diff.changed.push_back(entry);
            }
        }

        std::unordered_map<std::string, const ManifestEntry*> remoteByPath;
        remoteByPath.reserve(remote.entries.size());
        for (const auto& entry : remote.entries)
        {
            remoteByPath.emplace(entry.path, &entry);
        }

        for (const auto& entry : installed.entries)
        {
            if (!remoteByPath.contains(entry.path))
                diff.removedPaths.push_back(entry.path);
        }

        return diff;
    }

    std::vector<ManifestBlob> ManifestDiff::BlobsToInstall() const
    {
        std::vector<ManifestBlob> blobs;
        std::unordered_set<std::string> seen;
        const auto add = [&](const std::string& hash, const std::uint64_t size, const std::string& path)
        {
            if (hash.empty() || !seen.insert(hash).second)
                return;
            blobs.push_back({hash, size, path});
        };

        for (const std::vector<ManifestEntry>* group : {&added, &changed})
        {
            for (const ManifestEntry& entry : *group)
            {
                add(entry.hash, entry.size, entry.path);
                add(entry.metaHash, entry.metaSize, entry.path + ".meta");
            }
        }
        return blobs;
    }
}
