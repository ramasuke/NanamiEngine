#pragma once
#include <string>

#include "../AssetBase.h"
#include "../Factory/AssetFactory.h"

namespace NanamiEngine::Module::Asset
{
    class HlslFile final : public AssetBase
    {
    public:
        explicit HlslFile(std::string contentPath = "");
        [[nodiscard]] const Guid& GetGuid       () const override { return guid_; }
        [[nodiscard]] std::string GetContentPath() const override;

    private:
        [[serialize(0)]] std::string contentPath_;
        [[serialize(0)]] Guid        guid_;

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
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::HlslFile, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::HlslFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::HlslFile);
#pragma endregion
REGISTER_ASSET(HlslFile, ".hlsl")
