#pragma once
#include <string>

#include "DxLib.h"
#include "../../../LifeCycleCallback/EnableAsset/IEnablableAsset.h"
#include "../../AssetBase.h"
#include "../../Factory/AssetFactory.h"

namespace NanamiEngine::Module::Asset
{
    class TtfFontFile final : public AssetBase,
                              public LifeCycleCallback::IEnablableAsset
    {
    public:
        explicit TtfFontFile(std::string contentPath = "");

        [[nodiscard]] const Guid& GetGuid       () const override { return guid_; }
        [[nodiscard]] std::string GetContentPath() const override { return contentPath_; }
        [[nodiscard]] int         DxLibHandle   () const          { return dxLibHandle_;    }

    private:
        void OnEnableAsset() override;
        
        std::string contentPath_;
        std::string fontName_;
        int size_;
        int thickness_;
        int fontType_;

        Guid guid_;
        int dxLibHandle_ = -1;

#pragma region Serialization Function
    public:
        void OnDrawGui()
        {
            LibCore::ImGuiHelper::OnDrawInputField("fontName_", fontName_);
            LibCore::ImGuiHelper::OnDrawInputField("size_", size_);
            LibCore::ImGuiHelper::OnDrawInputField("thickness_", thickness_);
            LibCore::ImGuiHelper::OnDrawInputField("fontType_", fontType_);
            LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
            LibCore::ImGuiHelper::OnDrawInputField("dxLibHandle_", dxLibHandle_);
        }
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<AssetBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IEnablableAsset>(this));

            archive(CEREAL_NVP(fontName_));
            archive(CEREAL_NVP(size_));
            archive(CEREAL_NVP(thickness_));
            archive(CEREAL_NVP(fontType_));
            archive(CEREAL_NVP(guid_));
            archive(CEREAL_NVP(dxLibHandle_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<AssetBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IEnablableAsset>(this));

            if (version >= 0) archive(CEREAL_NVP(fontName_));
            if (version >= 0) archive(CEREAL_NVP(size_));
            if (version >= 0) archive(CEREAL_NVP(thickness_));
            if (version >= 0) archive(CEREAL_NVP(fontType_));
            if (version >= 0) archive(CEREAL_NVP(guid_));
            if (version >= 0) archive(CEREAL_NVP(dxLibHandle_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::TtfFontFile, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::TtfFontFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::TtfFontFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IEnablableAsset, NanamiEngine::Module::Asset::TtfFontFile);
#pragma endregion
REGISTER_ASSET(TtfFontFile, ".ttf")
