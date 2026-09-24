#pragma once
#include <memory>
#include <string>
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/NanamiUI/LayoutGroup/LayoutElement.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"

namespace GamePlay::Ui
{
    // 操作ガイドの1行。行の並びは親の VerticalLayoutGroup(行の占める割合は自身の LayoutElement)、行内の配置は子オブジェクトの Transform が決める。
    // 表示内容と透明度は ControlGuide が毎フレーム渡す
    class ControlGuideRow final : public Component::ComponentBase
    {
    public:
        struct Appearance
        {
            float slotRate            = 0.0f;
            float slideOffset_px      = 0.0f;
            int   bodyAlpha           = 0;
            int   labelShadowAlpha    = 0;
            int   accentGlowAlpha     = 0;
            int   glyphFlashAlpha     = 0;
            int   focusStripAlpha     = 0;
            int   focusArrowAlpha     = 0;
            int   focusCheckAlpha     = 0;
            float focusArrowOffset_px = 0.0f;
        };

        void SetContent(const std::weak_ptr<Asset::SpriteFile>& glyph, const std::string& label);
        /// ラベルの色は都度補間できないので、指され始め・外れた瞬間にだけ差し替える
        void SetFocused(bool isFocused);
        void Apply(const Appearance& appearance);

    private:
        void CatchParts();
        /// 長いラベルが強調の矢印に重ならないよう、収まらない分だけ横に詰める
        void FitLabelWidth();

        [[serialize(1)]] FIELD(GameObject::IGameObject) content_;
        [[serialize(1)]] FIELD(NanamiUi::BlendImageRenderer) strip_;
        [[serialize(1)]] FIELD(NanamiUi::BlendImageRenderer) accentGlow_;
        [[serialize(1)]] FIELD(NanamiUi::BlendImageRenderer) glyph_;
        [[serialize(1)]] FIELD(NanamiUi::BlendImageRenderer) glyphFlash_;
        [[serialize(1)]] FIELD(NanamiUi::TextRenderer) labelShadow_;
        [[serialize(1)]] FIELD(NanamiUi::TextRenderer) label_;
        [[serialize(2)]] FIELD(NanamiUi::BlendImageRenderer) focusStrip_;
        [[serialize(2)]] FIELD(NanamiUi::BlendImageRenderer) focusArrow_;
        [[serialize(2)]] FIELD(NanamiUi::BlendImageRenderer) focusCheck_;
        [[serialize(2)]] Color32 labelColor_ = Color32(255, 255, 247);
        [[serialize(2)]] Color32 focusLabelColor_ = Color32(255, 206, 104);
        [[serialize(3)]] float labelMaxWidth_px_ = 160.0f;

        bool isPartsCaught_ = false;
        std::weak_ptr<NanamiUi::LayoutElement> layoutElement_;
        glm::vec3 contentBasePos_ = glm::vec3(0.0f);
        glm::vec3 focusArrowBasePos_ = glm::vec3(0.0f);
        glm::vec3 labelBaseScale_ = glm::vec3(1.0f);
        glm::vec3 labelShadowBaseScale_ = glm::vec3(1.0f);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(content_));
            archive(CEREAL_NVP(strip_));
            archive(CEREAL_NVP(accentGlow_));
            archive(CEREAL_NVP(glyph_));
            archive(CEREAL_NVP(glyphFlash_));
            archive(CEREAL_NVP(labelShadow_));
            archive(CEREAL_NVP(label_));
            archive(CEREAL_NVP(focusStrip_));
            archive(CEREAL_NVP(focusArrow_));
            archive(CEREAL_NVP(focusCheck_));
            archive(CEREAL_NVP(labelColor_));
            archive(CEREAL_NVP(focusLabelColor_));
            archive(CEREAL_NVP(labelMaxWidth_px_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(content_));
            if (version >= 1) archive(CEREAL_NVP(strip_));
            if (version >= 1) archive(CEREAL_NVP(accentGlow_));
            if (version >= 1) archive(CEREAL_NVP(glyph_));
            if (version >= 1) archive(CEREAL_NVP(glyphFlash_));
            if (version >= 1) archive(CEREAL_NVP(labelShadow_));
            if (version >= 1) archive(CEREAL_NVP(label_));
            // v2 でチュートリアルが指す行の強調表示を足した
            if (version >= 2) archive(CEREAL_NVP(focusStrip_));
            if (version >= 2) archive(CEREAL_NVP(focusArrow_));
            if (version >= 2) archive(CEREAL_NVP(focusCheck_));
            if (version >= 2) archive(CEREAL_NVP(labelColor_));
            if (version >= 2) archive(CEREAL_NVP(focusLabelColor_));
            if (version >= 3) archive(CEREAL_NVP(labelMaxWidth_px_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::ControlGuideRow, 3);
