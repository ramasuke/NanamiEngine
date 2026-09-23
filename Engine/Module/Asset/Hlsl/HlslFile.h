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
        void OnRenamed(const std::string& newContentPath) override { contentPath_ = newContentPath; }

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
#pragma endregion
