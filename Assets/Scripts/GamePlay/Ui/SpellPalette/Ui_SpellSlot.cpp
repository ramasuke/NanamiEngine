#include "Ui_SpellSlot.h"

#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void SpellSlot::CatchParts()
    {
        if (isPartsCaught_)
            return;
        isPartsCaught_ = true;

        if (content_)
            contentBaseScale_ = content_->Transform().GetLocalScale();
    }

    void SpellSlot::SetSpell(const std::weak_ptr<Asset::SpriteFile>& icon, const std::string& costText)
    {
        CatchParts();

        if (icon_)     icon_    ->SetSprite(icon);
        if (costText_) costText_->SetText(costText);
    }

    void SpellSlot::SetManaLack(const bool isLacking)
    {
        CatchParts();

        if (costText_)
            costText_->SetTextColor(isLacking ? costLackColor_ : costColor_);
    }

    void SpellSlot::SetCooldownText(const std::string& text)
    {
        CatchParts();

        if (cooldownText_)
            cooldownText_->SetText(text);
    }

    void SpellSlot::SetGlyph(const std::weak_ptr<Asset::SpriteFile>& glyph, const glm::vec2& direction)
    {
        CatchParts();

        if (!glyph_)
            return;

        glyph_->SetSprite(glyph);
        const glm::vec3 glyphPos = glyph_->Transform().GetLocalPos();
        glyph_->Transform().SetLocalPos(glm::vec3(direction.x * glyphDistance_, direction.y * glyphDistance_, glyphPos.z));
    }

    void SpellSlot::Apply(const Appearance& appearance)
    {
        CatchParts();

        if (content_)
            content_->Transform().SetLocalScale(contentBaseScale_ * appearance.scale);

        if (socket_)        socket_       ->SetBlendRate(appearance.bodyAlpha);
        if (active_)        active_       ->SetBlendRate(appearance.activeAlpha);
        if (icon_)          icon_         ->SetBlendRate(appearance.iconAlpha);
        if (manaLackShade_) manaLackShade_->SetBlendRate(appearance.manaLackAlpha);
        if (cooldown_)
        {
            cooldown_->SetFillRate(appearance.cooldownRate);
            cooldown_->SetBlendRate(appearance.cooldownAlpha);
        }
        if (cooldownText_)  cooldownText_ ->SetBlendRate(appearance.cooldownRate > 0.0f ? appearance.textAlpha : 0);
        if (costPill_)      costPill_     ->SetBlendRate(appearance.textAlpha);
        if (costText_)      costText_     ->SetBlendRate(appearance.textAlpha);
        if (glyph_)         glyph_        ->SetBlendRate(appearance.glyphAlpha);
    }

    void SpellSlot::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("content_", content_);
        ImGuiHelper::OnDrawInputField("socket_", socket_);
        ImGuiHelper::OnDrawInputField("active_", active_);
        ImGuiHelper::OnDrawInputField("icon_", icon_);
        ImGuiHelper::OnDrawInputField("manaLackShade_", manaLackShade_);
        ImGuiHelper::OnDrawInputField("cooldown_", cooldown_);
        ImGuiHelper::OnDrawInputField("cooldownText_", cooldownText_);
        ImGuiHelper::OnDrawInputField("costPill_", costPill_);
        ImGuiHelper::OnDrawInputField("costText_", costText_);
        ImGuiHelper::OnDrawInputField("glyph_", glyph_);
        costColor_.OnDrawGui();
        costLackColor_.OnDrawGui();
        ImGuiHelper::OnDrawInputField("glyphDistance_", glyphDistance_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SpellSlot);
#pragma endregion
