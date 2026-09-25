#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Font/Ttf/TtfFontFile.h"
#include "../../Color/Color32.h"
#include "../../Component/ComponentBase.h"
#include "TextRenderer_TextAlign.h"

namespace NanamiEngine::Module::NanamiUi
{
    class NANAMI_API TextRenderer final : public Component::ComponentBase,
                               public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        ~TextRenderer() override;
        void SetText(const std::string& text);
        void SetFont(const std::shared_ptr<Asset::TtfFontFile>& font);
        void SetTextColor(const Color32& color);
        void SetWorldMode(bool isWorld);
        void SetTextAlign(TextAlign align);
        void SetBlendRate(int blendRate);
        /// Transform の拡大率を掛ける前の、最も長い行の幅(px)。フォントが無ければ 0
        [[nodiscard]] float MeasureTextWidth() const;

    private:
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        void UpdateTextTexture();
        void DrawScreenText() const;

    private:
        [[serialize(0)]] FIELD(Asset::TtfFontFile) fontFile_;
        [[serialize(0)]] int renderOrder_ = 0;
        [[serialize(0)]] std::string text_;
        [[serialize(0)]] Color32 textColor_;
        [[serialize(0)]] bool isWorldPos_ = false;
        [[serialize(0)]] TextAlign textAlign_ = TextAlign::Left;

        int blendRate_ = 255;

        // キャッシュ
        std::string cachedSjis_;
        bool isDirty_ = true;

        // MakeScreen
        int textScreen_ = -1;
        int screenW_ = 256;
        int screenH_ = 64;

#pragma region Serialization
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            archive(CEREAL_NVP(fontFile_));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(text_));
            archive(CEREAL_NVP(textColor_));
            if (version >= 1) archive(CEREAL_NVP(isWorldPos_));
            if (version >= 2) archive(CEREAL_NVP(textAlign_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            if (version >= 0) archive(CEREAL_NVP(fontFile_));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(text_));
            if (version >= 0) archive(CEREAL_NVP(textColor_));
            if (version >= 1) archive(CEREAL_NVP(isWorldPos_));
            if (version >= 2) archive(CEREAL_NVP(textAlign_));
            // version 3 だけが持っていた縁取り設定（TtfFontFile の edgeSize_/edgeColor_ に移行）は読み捨てる
            if (version == 3)
            {
                bool legacyIsOutlineEnabled = false;
                Color32 legacyOutlineColor;
                float legacyOutlineWidth = 0.0f;
                float legacyOutlineShadowOffsetY = 0.0f;
                archive(cereal::make_nvp("isOutlineEnabled_", legacyIsOutlineEnabled));
                archive(cereal::make_nvp("outlineColor_", legacyOutlineColor));
                archive(cereal::make_nvp("outlineWidth_", legacyOutlineWidth));
                archive(cereal::make_nvp("outlineShadowOffsetY_", legacyOutlineShadowOffsetY));
            }
            isDirty_ = true;
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::NanamiUi::TextRenderer, 4);
