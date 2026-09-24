#include "Ui_ControlGuideRow.h"

#include "Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void ControlGuideRow::CatchParts()
    {
        if (isPartsCaught_)
            return;
        isPartsCaught_ = true;

        if (const auto entity = Entity().lock())
            layoutElement_ = entity->Components().Catch<NanamiUi::LayoutElement>();
        if (content_)
            contentBasePos_ = content_->Transform().GetLocalPos();
        if (focusArrow_)
            focusArrowBasePos_ = focusArrow_->Transform().GetLocalPos();
        if (label_)
            labelBaseScale_ = label_->Transform().GetLocalScale();
        if (labelShadow_)
            labelShadowBaseScale_ = labelShadow_->Transform().GetLocalScale();
    }

    void ControlGuideRow::FitLabelWidth()
    {
        if (!label_)
            return;

        const float width = label_->MeasureTextWidth() * labelBaseScale_.x;
        const float squeeze = (width > labelMaxWidth_px_ && labelMaxWidth_px_ > 0.0f) ? labelMaxWidth_px_ / width : 1.0f;
        label_->Transform().SetLocalScale(glm::vec3(labelBaseScale_.x * squeeze, labelBaseScale_.y, labelBaseScale_.z));
        if (labelShadow_)
            labelShadow_->Transform().SetLocalScale(glm::vec3(labelShadowBaseScale_.x * squeeze, labelShadowBaseScale_.y, labelShadowBaseScale_.z));
    }

    void ControlGuideRow::SetContent(const std::weak_ptr<Asset::SpriteFile>& glyph, const std::string& label)
    {
        CatchParts();

        if (glyph_)       glyph_      ->SetSprite(glyph);
        if (glyphFlash_)  glyphFlash_ ->SetSprite(glyph);
        if (labelShadow_) labelShadow_->SetText(label);
        if (label_)       label_      ->SetText(label);
        FitLabelWidth();
    }

    void ControlGuideRow::SetFocused(const bool isFocused)
    {
        CatchParts();

        if (label_)
            label_->SetTextColor(isFocused ? focusLabelColor_ : labelColor_);
    }

    void ControlGuideRow::Apply(const Appearance& appearance)
    {
        CatchParts();

        if (const auto layoutElement = layoutElement_.lock())
            layoutElement->SetMainAxisRate(appearance.slotRate);
        if (content_)
            content_->Transform().SetLocalPos(contentBasePos_ + glm::vec3(appearance.slideOffset_px, 0.0f, 0.0f));
        if (focusArrow_)
            focusArrow_->Transform().SetLocalPos(focusArrowBasePos_ + glm::vec3(appearance.focusArrowOffset_px, 0.0f, 0.0f));

        if (strip_)       strip_      ->SetBlendRate(appearance.bodyAlpha);
        if (glyph_)       glyph_      ->SetBlendRate(appearance.bodyAlpha);
        if (accentGlow_)  accentGlow_ ->SetBlendRate(appearance.accentGlowAlpha);
        if (glyphFlash_)  glyphFlash_ ->SetBlendRate(appearance.glyphFlashAlpha);
        if (labelShadow_) labelShadow_->SetBlendRate(appearance.labelShadowAlpha);
        if (label_)       label_      ->SetBlendRate(appearance.bodyAlpha);
        if (focusStrip_)  focusStrip_ ->SetBlendRate(appearance.focusStripAlpha);
        if (focusArrow_)  focusArrow_ ->SetBlendRate(appearance.focusArrowAlpha);
        if (focusCheck_)  focusCheck_ ->SetBlendRate(appearance.focusCheckAlpha);
    }

    void ControlGuideRow::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("content_", content_);
        ImGuiHelper::OnDrawInputField("strip_", strip_);
        ImGuiHelper::OnDrawInputField("accentGlow_", accentGlow_);
        ImGuiHelper::OnDrawInputField("glyph_", glyph_);
        ImGuiHelper::OnDrawInputField("glyphFlash_", glyphFlash_);
        ImGuiHelper::OnDrawInputField("labelShadow_", labelShadow_);
        ImGuiHelper::OnDrawInputField("label_", label_);
        ImGuiHelper::OnDrawInputField("focusStrip_", focusStrip_);
        ImGuiHelper::OnDrawInputField("focusArrow_", focusArrow_);
        ImGuiHelper::OnDrawInputField("focusCheck_", focusCheck_);
        labelColor_.OnDrawGui();
        focusLabelColor_.OnDrawGui();
        ImGuiHelper::OnDrawInputField("labelMaxWidth_px_", labelMaxWidth_px_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ControlGuideRow);
#pragma endregion
