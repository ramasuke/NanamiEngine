#pragma once
#include "../AssetBase.h"
#include "../Factory/AssetFactory.h"
#include "../cereal/include/cereal/types/polymorphic.hpp"

namespace NanamiEngine::Module::Asset
{
    class SoundFile final : public AssetBase, public LifeCycleCallback::IEnablableAsset
    {
    public:
        explicit SoundFile(std::string contentPath = "");
        void OnEnableAsset() override;
        [[nodiscard]] const Guid& GetGuid       () const override { return guid_;         }
        [[nodiscard]] int         GetDxLibHandle() const          { return dxLibHandle_;  }
        [[nodiscard]] std::string GetContentPath() const override;

    private:
        std::string contentPath_;
        Guid guid_;
        int dxLibHandle_ = -1;

        int volume_ = 0;
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<AssetBase>(this));
            archive(CEREAL_NVP(contentPath_));
            archive(CEREAL_NVP(guid_));
            archive(CEREAL_NVP(volume_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<AssetBase>(this));
            if (version >= 0) archive(CEREAL_NVP(contentPath_));
            if (version >= 0) archive(CEREAL_NVP(guid_));
            if (version >= 0) archive(CEREAL_NVP(volume_));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::SoundFile, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::SoundFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::SoundFile);
#pragma endregion
REGISTER_ASSET(SoundFile, ".mp3")
