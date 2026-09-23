#pragma once
#include <string>
#include <unordered_map>

#include "../../../Color/Color32.h"
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
        ~TtfFontFile() override;
        TtfFontFile(const TtfFontFile&)            = delete;
        TtfFontFile& operator=(const TtfFontFile&) = delete;

        [[nodiscard]] const Guid& GetGuid       () const override { return guid_; }
        [[nodiscard]] std::string GetContentPath() const override { return contentPath_; }
        [[nodiscard]] int         DxLibHandle   () const          { return dxLibHandle_;    }
        [[nodiscard]] int         Size          () const          { return size_;           }
        [[nodiscard]] const Color32& EdgeColor  () const          { return edgeColor_;      }
        /** @brief 同じ書体を pixelSize で作ったハンドル。縮小描画だと細い線が欠けるので、画面上の大きさで作って原寸で描く */
        [[nodiscard]] int HandleForPixelSize(int pixelSize);

    private:
        void OnEnableAsset() override;
        void OnRenamed(const std::string& newContentPath) override { contentPath_ = newContentPath; }

        std::string contentPath_;
        std::string fontName_;
        int size_;
        int thickness_;
        int fontType_;
        // fontType_ が EDGE 系のときだけ有効
        int edgeSize_;
        Color32 edgeColor_;

        Guid guid_;
        int dxLibHandle_ = -1;
        std::unordered_map<int, int> sizedHandles_;
        /** AddFontResourceExA に成功したパス（空なら未登録）。Rename 後も登録時と同じパスで RemoveFontResourceExA するため別に持つ */
        std::string addedFontResourcePath_;

#pragma region Serialization Function
    public:
        void OnDrawGui()
        {
            LibCore::ImGuiHelper::OnDrawInputField("fontName_", fontName_);
            LibCore::ImGuiHelper::OnDrawInputField("size_", size_);
            LibCore::ImGuiHelper::OnDrawInputField("thickness_", thickness_);
            LibCore::ImGuiHelper::OnDrawInputField("fontType_", fontType_);
            LibCore::ImGuiHelper::OnDrawInputField("edgeSize_", edgeSize_);
            LibCore::ImGuiHelper::OnDrawInputField("edgeColor_", edgeColor_);
            LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
            LibCore::ImGuiHelper::OnDrawInputField("dxLibHandle_", dxLibHandle_);
        }
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<AssetBase>(this));
            if (version == 0) archive(cereal::base_class<LifeCycleCallback::IEnablableAsset>(this));
            archive(CEREAL_NVP(fontName_));
            archive(CEREAL_NVP(size_));
            archive(CEREAL_NVP(thickness_));
            archive(CEREAL_NVP(fontType_));
            archive(CEREAL_NVP(guid_));
            archive(CEREAL_NVP(contentPath_));
            archive(CEREAL_NVP(edgeSize_));
            archive(CEREAL_NVP(edgeColor_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<AssetBase>(this));
            if (version == 0) archive(cereal::base_class<LifeCycleCallback::IEnablableAsset>(this));
            if (version >= 0) archive(CEREAL_NVP(fontName_));
            if (version >= 0) archive(CEREAL_NVP(size_));
            if (version >= 0) archive(CEREAL_NVP(thickness_));
            if (version >= 0) archive(CEREAL_NVP(fontType_));
            if (version >= 0) archive(CEREAL_NVP(guid_));
            // version 1 まではハンドル値を保存していた。デストラクタで解放するため、古い値はメンバに入れず読み捨てる
            int legacyDxLibHandle = -1;
            if (version <= 1) archive(cereal::make_nvp("dxLibHandle_", legacyDxLibHandle));
            if (version >= 3) archive(CEREAL_NVP(contentPath_));
            if (version >= 4) archive(CEREAL_NVP(edgeSize_));
            if (version >= 4) archive(CEREAL_NVP(edgeColor_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::TtfFontFile, 4);
#pragma endregion
