#include "DistManifestBuilder.h"

#include <format>
#include <fstream>

#include <windows.h>

#include "DistScan.h"
#include "../Text/Utf8.h"
#include "../cereal/include/cereal/external/rapidjson/prettywriter.h"
#include "../cereal/include/cereal/external/rapidjson/stringbuffer.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    namespace
    {
        constexpr wchar_t DIST_MANIFEST_TEMP_SUFFIX[] = L".tmp";

        using DistManifestWriter = CEREAL_RAPIDJSON_NAMESPACE::PrettyWriter<CEREAL_RAPIDJSON_NAMESPACE::StringBuffer>;

        void DistManifestWriteString(DistManifestWriter& writer, const std::string& text)
        {
            writer.String(text.c_str(), static_cast<CEREAL_RAPIDJSON_NAMESPACE::SizeType>(text.size()));
        }
    }

    AssetManifest BuildManifest(const DistScanResult& scan, const std::string& version, const std::string& requiredClientVersion, const std::string& baseUrl)
    {
        AssetManifest manifest;
        manifest.schema                = DIST_MANIFEST_SCHEMA;
        manifest.version               = version;
        manifest.requiredClientVersion = requiredClientVersion;
        manifest.baseUrl               = baseUrl;
        manifest.entries               = scan.entries;
        return manifest;
    }

    std::uint64_t TotalBytesOf(const AssetManifest& manifest)
    {
        std::uint64_t total = 0;
        for (const ManifestEntry& entry : manifest.entries)
            total += entry.TotalSize();
        return total;
    }

    std::string SerializeManifest(const AssetManifest& manifest)
    {
        CEREAL_RAPIDJSON_NAMESPACE::StringBuffer buffer;
        DistManifestWriter writer(buffer);
        writer.SetIndent(' ', 2);

        writer.StartObject();
        writer.Key("schema");
        writer.Int(manifest.schema);
        writer.Key("version");
        DistManifestWriteString(writer, manifest.version);
        writer.Key("requiredClientVersion");
        DistManifestWriteString(writer, manifest.requiredClientVersion);
        writer.Key("baseUrl");
        DistManifestWriteString(writer, manifest.baseUrl);
        writer.Key("totalBytes");
        writer.Uint64(TotalBytesOf(manifest));
        writer.Key("entries");
        writer.StartArray();
        for (const ManifestEntry& entry : manifest.entries)
        {
            writer.StartObject();
            writer.Key("guid");
            DistManifestWriteString(writer, entry.guid);
            writer.Key("path");
            DistManifestWriteString(writer, entry.path);
            writer.Key("hash");
            DistManifestWriteString(writer, entry.hash);
            writer.Key("size");
            writer.Uint64(entry.size);
            writer.Key("metaHash");
            DistManifestWriteString(writer, entry.metaHash);
            writer.Key("metaSize");
            writer.Uint64(entry.metaSize);
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();

        return std::string(buffer.GetString(), buffer.GetSize()) + "\n";
    }

    std::string WriteManifestFile(const AssetManifest& manifest, const std::filesystem::path& path)
    {
        std::filesystem::path temporary = path;
        temporary += DIST_MANIFEST_TEMP_SUFFIX;
        {
            const std::string json = SerializeManifest(manifest);
            std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
            stream.write(json.data(), static_cast<std::streamsize>(json.size()));
            stream.close();
            if (!stream)
            {
                DeleteFileW(temporary.c_str());
                return "書き込めませんでした: " + WideToUtf8(temporary.wstring());
            }
        }
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            DeleteFileW(temporary.c_str());
            return "置き換えられませんでした: " + WideToUtf8(path.wstring());
        }
        return {};
    }

    std::string FormatBytes(const std::uint64_t count)
    {
        const double value = static_cast<double>(count);
        if (count < 1024)
            return std::format("{} B", count);
        if (count < 1024ull * 1024)
            return std::format("{:.1f} KB", value / 1024);
        if (count < 1024ull * 1024 * 1024)
            return std::format("{:.1f} MB", value / (1024 * 1024));
        return std::format("{:.2f} GB", value / (1024.0 * 1024 * 1024));
    }
}
