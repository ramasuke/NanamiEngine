#include "Ui_EventBoard_NoticeRow.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Sound/UiSoundBank.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    std::shared_ptr<Asset::SpriteFile> FindAnnouncementKindSprite(
        const std::vector<FIELD(Asset::SpriteFile)>& spritesByKind,
        const Asset::AnnouncementKind kind)
    {
        const auto index = static_cast<size_t>(kind);
        if (index >= spritesByKind.size())
            return nullptr;

        return spritesByKind[index].get();
    }

    void EventBoardNoticeRow::EnsureComponents()
    {
        if (selectButton_ && ticketRenderer_)
            return;

        selectButton_   = RequireComponent<NanamiUi::Button>();
        ticketRenderer_ = RequireComponent<Component::ImageRenderer>();
        baseScale_      = Transform().GetLocalScale();
    }

    void EventBoardNoticeRow::OnAwake()
    {
        EnsureComponents();

        selectButton_->OnHover().Subscribe([this](auto)
        {
            Sound::UiSoundBank::Play(uiSounds_, hoverSound_, Sound::UiSe::Cursor);
            isHovering_ = true;
            RefreshAppearance();
        }).AddTo(this);
        selectButton_->OnHoverExit().Subscribe([this](auto)
        {
            isHovering_ = false;
            RefreshAppearance();
        }).AddTo(this);
    }

    void EventBoardNoticeRow::Bind(const NoticeBoardEntry& entry)
    {
        EnsureComponents();

        const auto chip = FindAnnouncementKindSprite(kindChipSprites_, entry.announcement->Kind());
        kindChip_->SetEnable(chip != nullptr);
        if (chip)
            kindChip_->SetSprite(chip);
        dateText_  ->SetText(entry.dateText);
        titleText_ ->SetText(entry.announcement->Title());
        unreadSeal_->SetEnable(entry.isUnread);
    }

    void EventBoardNoticeRow::SubscribeOnClickSelectButton(std::function<void()> onClick)
    {
        EnsureComponents();
        selectButton_->OnClick().Subscribe([onClick](NanamiUi::MouseState)
        {
            onClick();
        }).AddTo(this);
    }

    void EventBoardNoticeRow::SetHighlighted(const bool isHighlighted)
    {
        isHighlighted_ = isHighlighted;
        RefreshAppearance();
    }

    void EventBoardNoticeRow::RefreshAppearance() const
    {
        ticketRenderer_->SetSprite(isHighlighted_ || isHovering_ ? selectedTicketSprite_.get() : unselectedTicketSprite_.get());
        waxSeal_->SetEnable(isHighlighted_);
        Transform().SetLocalScale(isHighlighted_ ? baseScale_ * selectedScale_ : baseScale_);
    }

    void EventBoardNoticeRow::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("kindChip_", kindChip_);
        ImGuiHelper::OnDrawInputField("dateText_", dateText_);
        ImGuiHelper::OnDrawInputField("titleText_", titleText_);
        ImGuiHelper::OnDrawInputField("unreadSeal_", unreadSeal_);
        ImGuiHelper::OnDrawInputField("waxSeal_", waxSeal_);
        ImGuiHelper::OnDrawInputField("kindChipSprites_", kindChipSprites_, [this]
        {
            if (ImGui::Button("Add Sprite"))
            {
                kindChipSprites_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("selectedTicketSprite_", selectedTicketSprite_);
        ImGuiHelper::OnDrawInputField("unselectedTicketSprite_", unselectedTicketSprite_);
        ImGuiHelper::OnDrawInputField("hoverSound_", hoverSound_);
        ImGuiHelper::OnDrawInputField("selectedScale_", selectedScale_);
        ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardNoticeRow);
#pragma endregion
