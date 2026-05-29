#pragma once
#include "../AssetBase.h"
#include "../../LifeCycleCallback/EnableAsset/IEnablableAsset.h"
#include "../Factory/AssetFactory.h"

namespace NanamiEngine::Module::Asset
{
    class MovieFile final : public AssetBase
    {
    public:
        explicit MovieFile(const std::string& contentPath = "");
        [[nodiscard]] const Guid& GetGuid       () const override;
        [[nodiscard]] int         LoadDxLibHandle() const;
        [[nodiscard]] std::string GetContentPath() const override;

    private:
        std::string contentPath_;
        Guid guid_;
        
#pragma region Serialization Function
    public:
    void OnDrawGui() override;

    template<class Archive>
    void save(Archive& archive, const std::uint32_t version) const {
        archive(cereal::base_class<AssetBase>(this));
        archive(CEREAL_NVP(contentPath_));
        archive(CEREAL_NVP(guid_));
    }
    
    template<class Archive>
    void load(Archive& archive, const std::uint32_t version) {
        archive(cereal::base_class<AssetBase>(this));
        if (version >= 0) archive(CEREAL_NVP(contentPath_));
        if (version >= 0) archive(CEREAL_NVP(guid_));
    }
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::MovieFile, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::MovieFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::MovieFile);
#pragma endregion
REGISTER_ASSET(MovieFile, ".mp4")
