#pragma once
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Font/Ttf/TtfFontFile.h"
#include "../../Color/Color32.h"
#include "../../Component/ComponentBase.h"
#include "TextRenderer_TextAlign.h"

namespace NanamiEngine::Module::NanamiUi
{
    class TextRenderer final : public Component::ComponentBase,
                               public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        ~TextRenderer() override;
        void SetText(const std::string& text);
        void SetFont(const std::shared_ptr<Asset::TtfFontFile>& font);
        void SetTextColor(const Color32& color);
        void SetWorldMode(bool isWorld);
        void SetTextAlign(TextAlign align);

    private:
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        void UpdateTextTexture();
        void DrawScreenText(float offsetX, float offsetY, int dxColor) const;

    private:
        [[serialize(0)]] FIELD(Asset::TtfFontFile) fontFile_;
        [[serialize(0)]] int renderOrder_ = 0;
        [[serialize(0)]] std::string text_;
        [[serialize(0)]] Color32 textColor_;
        [[serialize(0)]] bool isWorldPos_ = false;
        [[serialize(0)]] TextAlign textAlign_ = TextAlign::Left;
        // スクリーン座標モードのみ：本文の周囲8方向＋下方向に outlineColor_ で重ね描きして縁取りにする
        [[serialize(3)]] bool isOutlineEnabled_ = false;
        [[serialize(3)]] Color32 outlineColor_ = Color32(6, 20, 26);
        [[serialize(3)]] float outlineWidth_ = 1.7f;
        // 縁取りに加えて outlineColor_ で真下にずらして描く影の量
        [[serialize(3)]] float outlineShadowOffsetY_ = 2.2f;

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
            if (version >= 3) archive(CEREAL_NVP(isOutlineEnabled_));
            if (version >= 3) archive(CEREAL_NVP(outlineColor_));
            if (version >= 3) archive(CEREAL_NVP(outlineWidth_));
            if (version >= 3) archive(CEREAL_NVP(outlineShadowOffsetY_));
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
            if (version >= 3) archive(CEREAL_NVP(isOutlineEnabled_));
            if (version >= 3) archive(CEREAL_NVP(outlineColor_));
            if (version >= 3) archive(CEREAL_NVP(outlineWidth_));
            if (version >= 3) archive(CEREAL_NVP(outlineShadowOffsetY_));
            isDirty_ = true;
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::NanamiUi::TextRenderer, 3)
