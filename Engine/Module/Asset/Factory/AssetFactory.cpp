#include "AssetFactory.h"

namespace NanamiEngine::Module::Asset
{
    void AssetFactory::RegisterCreatableAssetExtension(
        const std::string& assetNameLabel,
        const std::string& extensionLabel)
    {
        if (std::ranges::any_of(creatableAssetsData_, [&](const auto& p) { return p.first == assetNameLabel && p.second == extensionLabel; }))
            return;
    
        
        creatableAssetsData_.emplace_back(assetNameLabel, extensionLabel);
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
