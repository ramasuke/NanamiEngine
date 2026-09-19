#include "AssetFactory.h"

namespace NanamiEngine::Module::Asset
{
    void AssetFactory::RegisterCreatableAssetExtension(
        const std::string& assetNameLabel,
        const std::string& extensionLabel,
        const std::string& categoryLabel)
    {
        if (std::ranges::any_of(creatableAssetsData_, [&](const CreatableAsset& asset) { return asset.name == assetNameLabel && asset.extension == extensionLabel; }))
            return;


        creatableAssetsData_.push_back(CreatableAsset{ assetNameLabel, extensionLabel, categoryLabel });
    }
    
    bool AssetFactory::TryCreate(const std::string& filePath,
                                 std::shared_ptr<AssetBase>& outAsset) const
    {
        for (const auto& factory : factories_)
        {
            if (factory(filePath, outAsset) && outAsset)
                return true;
        }
    
        outAsset = nullptr;
        return false;
    }

    bool AssetFactory::IsRegisteredExtension(const std::string& filePath) const
    {
        return std::ranges::any_of(registeredExtensions_, [&filePath](const std::string& extension)
        {
            return LibCore::FilePath::IsExtension(filePath, extension);
        });
    }
    
    std::shared_ptr<AssetBase> AssetFactory::Load(
        const std::string& filePath) const
    {
        for (const auto& loader : loaderers_)
        {
            if (auto result = loader(filePath); result)
                return result;
        }
        return nullptr;
    }
}
