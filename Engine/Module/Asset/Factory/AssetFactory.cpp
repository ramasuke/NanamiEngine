#include "AssetFactory.h"

namespace NanamiEngine::Module::Asset
{
    void AssetFactory::RegisterCreatableAssetExtension(
        const std::string& assetNameLabel,
        const std::string& extensionLabel,
        const std::string& categoryLabel,
        void* const module)
    {
        if (std::ranges::any_of(creatableAssetsData_, [&](const auto& registered) { return registered.entry.name == assetNameLabel && registered.entry.extension == extensionLabel; }))
            return;
        
        creatableAssetsData_.push_back({ CreatableAsset{ assetNameLabel, extensionLabel, categoryLabel }, module });
    }

    bool AssetFactory::TryCreate(const std::string& filePath,
                                 std::shared_ptr<AssetBase>& outAsset) const
    {
        for (const auto& factory : factories_)
        {
            if (factory.entry(filePath, outAsset) && outAsset)
                return true;
        }
        outAsset = nullptr;
        return false;
    }

    bool AssetFactory::IsRegisteredExtension(const std::string& filePath) const
    {
        return std::ranges::any_of(registeredExtensions_, [&filePath](const auto& registered)
        {
            return LibCore::FilePath::IsExtension(filePath, registered.entry);
        });
    }

    std::shared_ptr<AssetBase> AssetFactory::Load(
        const std::string& filePath) const
    {
        for (const auto& loader : loaderers_)
        {
            if (auto result = loader.entry(filePath); result)
                return result;
        }
        return nullptr;
    }

    std::vector<CreatableAsset> AssetFactory::CreatableAssets() const
    {
        std::vector<CreatableAsset> result;
        result.reserve(creatableAssetsData_.size());
        for (const auto& registered : creatableAssetsData_)
            result.push_back(registered.entry);
        return result;
    }

    std::size_t AssetFactory::UnregisterModule(const void* module)
    {
        const auto ofModule = [module](const auto& registered) { return registered.module == module; };
        return std::erase_if(factories_, ofModule)
             + std::erase_if(registeredExtensions_, ofModule)
             + std::erase_if(loaderers_, ofModule)
             + std::erase_if(creatableAssetsData_, ofModule);
    }
}

NanamiEngine::Module::Asset::AssetFactory& NanamiEngine::Module::Asset::AssetFactory::Instance()
{
    static AssetFactory instance;
    return instance;
}
