#pragma once
#include <string>

#include "../../Guid/Guid.h"
#include "../../LifeCycleCallback/EnableAsset/IEnablableAsset.h"
#include "../AssetBase.h"
#include "../Factory/AssetFactory.h"

namespace NanamiEngine::Module::Asset
{
    class SpriteFile final : public AssetBase,
                             public LifeCycleCallback::IEnablableAsset
    {
    public:
        explicit SpriteFile(std::string contentPath = "");
        void OnEnableAsset() override;
        [[nodiscard]] const Guid& GetGuid        () const override { return guid_;     }
        [[nodiscard]] int         GetDxLibHandle () const          { return dxLibId_;  }
        [[nodiscard]] std::string GetContentPath () const override;

    private:
        [[nodiscard]] int         LoadGraph() const;
        
        std::string contentPath_;
        Guid guid_;
        int dxLibId_ = -1;
#pragma region Serialization Function
public:
void OnDrawGui() override;

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<AssetBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IEnablableAsset>(this));
    archive(CEREAL_NVP(contentPath_));  
    archive(CEREAL_NVP(guid_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<AssetBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IEnablableAsset>(this));
    if (version >= 0) archive(CEREAL_NVP(contentPath_));
    if (version >= 0) archive(CEREAL_NVP(guid_));
}
#pragma endregion
};
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SpriteFile, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::SpriteFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::SpriteFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IEnablableAsset, NanamiEngine::Module::Asset::SpriteFile);
#pragma endregion
REGISTER_ASSET(SpriteFile, ".png")
