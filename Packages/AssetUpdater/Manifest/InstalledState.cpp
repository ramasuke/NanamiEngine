#include "InstalledState.h"

#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <fstream>
#include <optional>
#include <vector>

#include <windows.h>

#include "../Hash/Sha256.h"
#include "../Text/Utf8.h"
#include "../cereal/include/cereal/external/rapidjson/stringbuffer.h"
#include "../cereal/include/cereal/external/rapidjson/prettywriter.h"

namespace NanamiEngine::AssetUpdater
{
    namespace
    {
        // tools/dist/manifest.py の SCHEMA と揃える
        constexpr int     INSTALLED_STATE_SCHEMA        = 1;
        // NOTE: 配信した版ではなく書き出したときの Assets/ そのものなので、版名の代わり
        constexpr char    INSTALLED_STATE_VERSION[]     = "local";
        constexpr wchar_t INSTALLED_STATE_META_SUFFIX[] = L".meta";
        constexpr wchar_t INSTALLED_STATE_TEMP_SUFFIX[] = L".tmp";

        struct InstalledStateEntry
        {
            std::string   path;
            std::string   hash;
            std::uint64_t size     = 0;
            std::string   metaHash;
            std::uint64_t metaSize = 0;
        };

        bool InstalledStateIsMeta(const std::filesystem::path& path)
        {
            std::wstring extension = path.extension().wstring();
            std::ranges::transform(extension, extension.begin(), [](const wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
            return extension == INSTALLED_STATE_META_SUFFIX;
        }

        std::string InstalledStateSerialize(const std::vector<InstalledStateEntry>& entries)
        {
            CEREAL_RAPIDJSON_NAMESPACE::StringBuffer buffer;
            CEREAL_RAPIDJSON_NAMESPACE::PrettyWriter<CEREAL_RAPIDJSON_NAMESPACE::StringBuffer> writer(buffer);
            writer.SetIndent(' ', 2);

            writer.StartObject();
            writer.Key("schema");
            writer.Int(INSTALLED_STATE_SCHEMA);
            writer.Key("version");
            writer.String(INSTALLED_STATE_VERSION);
            writer.Key("entries");
            writer.StartArray();
            for (const InstalledStateEntry& entry : entries)
            {
                writer.StartObject();
                writer.Key("path");
                writer.String(entry.path.c_str(), static_cast<CEREAL_RAPIDJSON_NAMESPACE::SizeType>(entry.path.size()));
                writer.Key("hash");
                writer.String(entry.hash.c_str(), static_cast<CEREAL_RAPIDJSON_NAMESPACE::SizeType>(entry.hash.size()));
                writer.Key("size");
                writer.Uint64(entry.size);
                writer.Key("metaHash");
                writer.String(entry.metaHash.c_str(), static_cast<CEREAL_RAPIDJSON_NAMESPACE::SizeType>(entry.metaHash.size()));
                writer.Key("metaSize");
                writer.Uint64(entry.metaSize);
                writer.EndObject();
            }
            writer.EndArray();
            writer.EndObject();

            return std::string(buffer.GetString(), buffer.GetSize()) + "\n";
        }
    }

    InstalledStateResult WriteInstalledState(const std::filesystem::path& gameRoot, const std::filesystem::path& installedState,
                                             const std::function<bool()>& isCanceled)
    {
        InstalledStateResult result;
        const std::filesystem::path assetsDirectory = gameRoot / L"Assets";

        std::error_code error;
        if (!std::filesystem::is_directory(assetsDirectory, error))
        {
            result.error = "Assets/ がありません: " + WideToUtf8(assetsDirectory.wstring());
            return result;
        }

        std::vector<std::filesystem::path> files;
        for (auto it = std::filesystem::recursive_directory_iterator(assetsDirectory); it != std::filesystem::recursive_directory_iterator(); ++it)
        {
            if (it->is_directory())
            {
                // ジャンクションの先は配信の対象にしない
                if (GetFileAttributesW(it->path().c_str()) & FILE_ATTRIBUTE_REPARSE_POINT)
                    it.disable_recursion_pending();
                continue;
            }
            // .meta は本体のエントリに畳む。本体の無い .meta は manifest.json にも出ない
            if (it->is_regular_file() && !InstalledStateIsMeta(it->path()))
                files.push_back(it->path());
        }

        std::vector<InstalledStateEntry> entries;
        entries.reserve(files.size());
        for (const std::filesystem::path& file : files)
        {
            if (isCanceled && isCanceled())
            {
                result.canceled = true;
                return result;
            }

            InstalledStateEntry entry;
            entry.path = WideToUtf8(file.lexically_relative(gameRoot).generic_wstring());

            const std::optional<std::string> hash = Sha256OfFile(file);
            if (!hash)
            {
                result.error = "ハッシュを取れませんでした: " + entry.path;
                return result;
            }
            entry.hash = *hash;
            entry.size = std::filesystem::file_size(file);

            std::filesystem::path meta = file;
            meta += INSTALLED_STATE_META_SUFFIX;
            if (std::filesystem::is_regular_file(meta, error))
            {
                const std::optional<std::string> metaHash = Sha256OfFile(meta);
                if (!metaHash)
                {
                    result.error = "ハッシュを取れませんでした: " + entry.path + ".meta";
                    return result;
                }
                entry.metaHash = *metaHash;
                entry.metaSize = std::filesystem::file_size(meta);
            }
            entries.push_back(std::move(entry));
        }
        std::ranges::sort(entries, {}, &InstalledStateEntry::path);

        // 途中で失敗しても、前の installed.json を壊さない
        std::filesystem::path temporary = installedState;
        temporary += INSTALLED_STATE_TEMP_SUFFIX;
        {
            const std::string json = InstalledStateSerialize(entries);
            std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
            stream.write(json.data(), static_cast<std::streamsize>(json.size()));
            stream.close();
            if (!stream)
            {
                DeleteFileW(temporary.c_str());
                result.error = "書き込めませんでした: " + WideToUtf8(temporary.wstring());
                return result;
            }
        }
        if (!MoveFileExW(temporary.c_str(), installedState.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            DeleteFileW(temporary.c_str());
            result.error = "置き換えられませんでした: " + WideToUtf8(installedState.wstring());
            return result;
        }

        result.ok         = true;
        result.entryCount = entries.size();
        return result;
    }
}
