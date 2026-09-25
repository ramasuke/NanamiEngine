#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

#include "../../LifeCycleCallback/EnableAsset/IEnablableAsset.h"
#include "../AssetBase.h"
#include "../Factory/AssetFactory.h"

namespace NanamiEngine::Module::Asset
{
    class NANAMI_API HlslVsFile final : public AssetBase,
                             public LifeCycleCallback::IEnablableAsset
    {
    public:
        explicit HlslVsFile(const std::string& contentPath = "");
        ~HlslVsFile() override;
        HlslVsFile(const HlslVsFile&)            = delete;
        HlslVsFile& operator=(const HlslVsFile&) = delete;
        [[nodiscard]] const Guid& GetGuid       () const override;
        [[nodiscard]] int         GetVsHandle    () const;
        [[nodiscard]] std::string GetContentPath () const override;

    private:
        void OnEnableAsset() override;
        void OnRenamed(const std::string& newContentPath) override { contentPath_ = newContentPath; }

        std::string contentPath_;
        Guid        guid_;
        int         vsHandle_ = -1;

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
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::HlslVsFile, 0);
#pragma endregion
