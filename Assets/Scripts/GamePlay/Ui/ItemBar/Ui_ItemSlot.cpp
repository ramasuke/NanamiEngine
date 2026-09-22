#include "Ui_ItemSlot.h"

#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Ui
{
    void ItemSlot::CatchParts()
    {
        if (isPartsCaught_)
            return;
        isPartsCaught_ = true;

        if (content_)
            contentBaseScale_ = content_->Transform().GetLocalScale();
    }

    void ItemSlot::SetContent(const std::weak_ptr<Asset::SpriteFile>& icon, const std::string& countText)
    {
        CatchParts();

        if (icon_)      icon_     ->SetSprite(icon);
        if (countText_) countText_->SetText(countText);
    }

    void ItemSlot::SetSelected(const bool isSelected)
    {
        CatchParts();

        if (countText_)
            countText_->SetTextColor(isSelected ? countSelectedColor_ : countColor_);
    }

    void ItemSlot::Apply(const Appearance& appearance)
    {
        CatchParts();

        if (content_)
            content_->Transform().SetLocalScale(contentBaseScale_ * appearance.scale);

        if (backing_)       backing_      ->SetBlendRate(appearance.bodyAlpha);
        if (icon_)          icon_         ->SetBlendRate(appearance.iconAlpha);
        if (frame_)         frame_        ->SetBlendRate(appearance.frameAlpha);
        if (selectedFrame_) selectedFrame_->SetBlendRate(appearance.selectedFrameAlpha);
        if (selectGlow_)    selectGlow_   ->SetBlendRate(appearance.selectGlowAlpha);
        if (countPill_)     countPill_    ->SetBlendRate(appearance.countAlpha);
        if (countText_)     countText_    ->SetBlendRate(appearance.countAlpha);
    }

    void ItemSlot::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("content_", content_);
        ImGuiHelper::OnDrawInputField("backing_", backing_);
        ImGuiHelper::OnDrawInputField("icon_", icon_);
        ImGuiHelper::OnDrawInputField("frame_", frame_);
        ImGuiHelper::OnDrawInputField("selectedFrame_", selectedFrame_);
        ImGuiHelper::OnDrawInputField("selectGlow_", selectGlow_);
        ImGuiHelper::OnDrawInputField("countPill_", countPill_);
        ImGuiHelper::OnDrawInputField("countText_", countText_);
        countColor_.OnDrawGui();
        countSelectedColor_.OnDrawGui();
    }
}
