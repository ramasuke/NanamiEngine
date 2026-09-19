#include "Engine_Asset_AssetPreloader.h"

#include <cctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_set>

#include "Engine_Asset_IPreloadableAsset.h"
#include "../AssetBase.h"
#include "../../Guid/Guid.h"
#include "../../../Core/Application/ApplicationBase.h"
#include "../../../Core/FileSystem/Directory/Directory.h"
#include "../../../Core/Object/Registry/ObjectRegistry.h"

namespace
{
    constexpr std::size_t ASSET_PRELOADER_GUID_LENGTH = 36;

    /** 本体に GUID を書かない形式。これらは .meta だけ読む */
    constexpr std::string_view ASSET_PRELOADER_BINARY_EXTENSIONS[] = {
        ".mv1", ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds",
        ".mp3", ".wav", ".ogg",
        ".efkefc", ".efkmodel",
        ".ttf", ".otf", ".ttc",
        ".mp4",
        ".vso", ".pso",
        ".fbx", ".blend",
    };

    struct AssetPreloaderScanCacheEntry
    {
        std::filesystem::file_time_type bodyWriteTime;
        std::filesystem::file_time_type metaWriteTime;
        std::vector<std::string> guids;
    };

    std::mutex& AssetPreloaderScanCacheMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    std::unordered_map<std::string, AssetPreloaderScanCacheEntry>& AssetPreloaderScanCache()
    {
        static std::unordered_map<std::string, AssetPreloaderScanCacheEntry> cache;
        return cache;
    }

    std::string AssetPreloaderLowerExtension(const std::string& path)
    {
        const std::size_t dot       = path.find_last_of('.');
        const std::size_t separator = path.find_last_of("/\\");
        if (dot == std::string::npos || (separator != std::string::npos && dot < separator))
            return "";

        std::string extension = path.substr(dot);
        for (char& c : extension)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return extension;
    }

    bool AssetPreloaderIsBinaryFile(const std::string& path)
    {
        const std::string extension = AssetPreloaderLowerExtension(path);
        for (const std::string_view binaryExtension : ASSET_PRELOADER_BINARY_EXTENSIONS)
        {
            if (extension == binaryExtension)
                return true;
        }
        return false;
    }

    bool AssetPreloaderIsSceneFile(const std::string& path)
    {
        return AssetPreloaderLowerExtension(path) == ".scene";
    }

    bool AssetPreloaderIsHex(const char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    }

    /** @brief begin から 8-4-4-4-12 形式の GUID が始まっているか */
    bool AssetPreloaderIsGuidAt(const std::string_view text, const std::size_t begin)
    {
        for (std::size_t i = 0; i < ASSET_PRELOADER_GUID_LENGTH; ++i)
        {
            const char c = text[begin + i];
            const bool isHyphenPosition = i == 8 || i == 13 || i == 18 || i == 23;
            if (isHyphenPosition ? c != '-' : !AssetPreloaderIsHex(c))
                return false;
        }
        return true;
    }

    void AssetPreloaderAppendGuids(const std::string_view text, std::vector<std::string>& outGuids)
    {
        std::size_t i = 0;
        while (i + ASSET_PRELOADER_GUID_LENGTH <= text.size())
        {
            // 区切りの位置を先に見て、GUID ではない位置を安く飛ばす
            if (text[i + 8] == '-' && text[i + 23] == '-' && AssetPreloaderIsGuidAt(text, i))
            {
                outGuids.emplace_back(text.substr(i, ASSET_PRELOADER_GUID_LENGTH));
                i += ASSET_PRELOADER_GUID_LENGTH;
            }
            else
            {
                ++i;
            }
        }
    }

    bool AssetPreloaderReadFile(const std::string& path, std::string& outText)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
            return false;

        stream.seekg(0, std::ios::end);
        const std::streamoff size = stream.tellg();
        outText.clear();
        if (size <= 0)
            return true;

        outText.resize(static_cast<std::size_t>(size));
        stream.seekg(0, std::ios::beg);
        stream.read(outText.data(), size);
        return true;
    }

    std::filesystem::file_time_type AssetPreloaderWriteTime(const std::string& path)
    {
        // 先読みの失敗でシーンの読み込みまで失敗させないよう、パスの変換で投げられる例外もここで止める
        try
        {
            std::error_code error;
            const auto writeTime = std::filesystem::last_write_time(std::filesystem::path(path), error);
            return error ? (std::filesystem::file_time_type::min)() : writeTime;
        }
        catch (const std::exception&)
        {
            return (std::filesystem::file_time_type::min)();
        }
    }

    /** @brief path とその .meta に書かれた GUID を返す。どちらの更新時刻も変わっていなければ前回の結果を使う */
    std::vector<std::string> AssetPreloaderScanReferencedGuids(const std::string& path)
    {
        const std::string metaPath = path + ".meta";
        const bool isBinary = AssetPreloaderIsBinaryFile(path);
        const auto bodyWriteTime = isBinary ? (std::filesystem::file_time_type::min)() : AssetPreloaderWriteTime(path);
        const auto metaWriteTime = AssetPreloaderWriteTime(metaPath);
        {
            const std::scoped_lock lock(AssetPreloaderScanCacheMutex());
            const auto& cache = AssetPreloaderScanCache();
            if (const auto it = cache.find(path);
                it != cache.end() && it->second.bodyWriteTime == bodyWriteTime && it->second.metaWriteTime == metaWriteTime)
            {
                return it->second.guids;
            }
        }

        std::vector<std::string> guids;
        std::string text;
        if (!isBinary && AssetPreloaderReadFile(path, text))
            AssetPreloaderAppendGuids(text, guids);
        if (AssetPreloaderReadFile(metaPath, text))
            AssetPreloaderAppendGuids(text, guids);

        const std::scoped_lock lock(AssetPreloaderScanCacheMutex());
        AssetPreloaderScanCache()[path] = AssetPreloaderScanCacheEntry{ bodyWriteTime, metaWriteTime, guids };
        return guids;
    }

    void AssetPreloaderForEachAsset(
        NanamiEngine::Core::FileSystem::Directory& directory,
        const std::function<void(const std::shared_ptr<NanamiEngine::Module::Asset::AssetBase>&)>& action)
    {
        for (auto& file : directory.Files())
        {
            if (const auto& content = file.GetContent())
                action(content);
        }

        for (auto& child : directory.GetDirectories())
        {
            AssetPreloaderForEachAsset(child, action);
        }
    }
}

namespace NanamiEngine::Module::Asset
{
    AssetPreloader::Index AssetPreloader::BuildIndex()
    {
        Index index;
        AssetPreloaderForEachAsset(Core::Application::ApplicationBase::AssetsDirectory(), [&index](const std::shared_ptr<AssetBase>& asset)
        {
            index.emplace(asset->GetGuid().Value(), asset->GetContentPath());
        });
        return index;
    }

    std::vector<std::string> AssetPreloader::CollectDependencies(const Index& index, const std::string& sceneFilePath)
    {
        std::vector<std::string> dependencies;
        std::unordered_set<std::string> visited;
        std::vector<std::string> pendingPaths{ sceneFilePath };
        while (!pendingPaths.empty())
        {
            const std::string path = std::move(pendingPaths.back());
            pendingPaths.pop_back();

            for (const auto& guid : AssetPreloaderScanReferencedGuids(path))
            {
                const auto it = index.find(guid);
                if (it == index.end() || !visited.insert(guid).second)
                    continue;

                dependencies.push_back(guid);
                // 遷移先などで参照しているシーンの中身は、そのシーンを開くときに読む
                if (!AssetPreloaderIsSceneFile(it->second))
                    pendingPaths.push_back(it->second);
            }
        }
        return dependencies;
    }

    void AssetPreloader::RequestLoads(const std::vector<std::string>& guids)
    {
        const auto& registry = Core::Application::ApplicationBase::ObjectRegistry();
        for (const auto& guid : guids)
        {
            if (const auto asset = registry.Catch<IPreloadableAsset>(Guid(guid)).lock())
                asset->RequestLoad();
        }
    }

    void AssetPreloader::RequestForScene(const std::string& sceneFilePath)
    {
        if (sceneFilePath.empty())
            return;

        RequestLoads(CollectDependencies(BuildIndex(), sceneFilePath));
    }

    void AssetPreloader::ReleaseUnused(const std::vector<std::string>& keptSceneFilePaths)
    {
        const Index index = BuildIndex();
        std::unordered_set<std::string> keptGuids;
        for (const auto& sceneFilePath : keptSceneFilePaths)
        {
            for (auto& guid : CollectDependencies(index, sceneFilePath))
            {
                keptGuids.insert(std::move(guid));
            }
        }

        AssetPreloaderForEachAsset(Core::Application::ApplicationBase::AssetsDirectory(), [&keptGuids](const std::shared_ptr<AssetBase>& asset)
        {
            if (keptGuids.contains(asset->GetGuid().Value()))
                return;

            if (const auto preloadable = std::dynamic_pointer_cast<IPreloadableAsset>(asset))
                preloadable->Unload();
        });
    }
}
