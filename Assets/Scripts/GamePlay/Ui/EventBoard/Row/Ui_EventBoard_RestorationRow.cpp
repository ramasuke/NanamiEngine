#include "Ui_EventBoard_RestorationRow.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Sound/SoundPlayer.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void EventBoardRestorationRow::EnsureComponents()
    {
        if (selectButton_ && ticketRenderer_)
            return;

        selectButton_   = RequireComponent<NanamiUi::Button>();
        ticketRenderer_ = RequireComponent<Component::ImageRenderer>();
        baseScale_      = Transform().GetLocalScale();
    }

    void EventBoardRestorationRow::OnAwake()
    {
        EnsureComponents();

        selectButton_->OnHover().Subscribe([this](auto)
        {
            if (const auto sound = hoverSound_.get())
                Sound::SoundPlayer::PlaySe(*sound, Sound::SoundPlayer::Position());
            isHovering_ = true;
            RefreshAppearance();
        }).AddTo(this);
        selectButton_->OnHoverExit().Subscribe([this](auto)
        {
            isHovering_ = false;
            RefreshAppearance();
        }).AddTo(this);
    }

    void EventBoardRestorationRow::Bind(const RestorationBoardEntry& entry)
    {
        EnsureComponents();

        nameText_->SetText(entry.facility->Name());
        costText_->SetText(entry.costText);
        noteText_->SetText(entry.noteText);
        const bool isRefused = entry.state == RestorationBoardState::Open && !entry.isAffordable;
        noteText_->SetTextColor(isRefused ? refusedNoteColor_ : defaultNoteColor_);

        const auto stamp = entry.state == RestorationBoardState::Restored ? restoredStampSprite_.get()
                         : entry.state == RestorationBoardState::Locked   ? lockedStampSprite_.get()
                         : nullptr;
        stateStamp_->SetEnable(stamp != nullptr);
        if (stamp)
            stateStamp_->SetSprite(stamp);
    }

    void EventBoardRestorationRow::SubscribeOnClickSelectButton(std::function<void()> onClick)
    {
        EnsureComponents();
        selectButton_->OnClick().Subscribe([onClick](NanamiUi::MouseState)
        {
            onClick();
        }).AddTo(this);
    }

    void EventBoardRestorationRow::SetHighlighted(const bool isHighlighted)
    {
        isHighlighted_ = isHighlighted;
        RefreshAppearance();
    }

    void EventBoardRestorationRow::RefreshAppearance() const
    {
        ticketRenderer_->SetSprite(isHighlighted_ || isHovering_ ? selectedTicketSprite_.get() : unselectedTicketSprite_.get());
        waxSeal_->SetEnable(isHighlighted_);
        Transform().SetLocalScale(isHighlighted_ ? baseScale_ * selectedScale_ : baseScale_);
    }

    void EventBoardRestorationRow::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("nameText_", nameText_);
        ImGuiHelper::OnDrawInputField("costText_", costText_);
        ImGuiHelper::OnDrawInputField("noteText_", noteText_);
        ImGuiHelper::OnDrawInputField("stateStamp_", stateStamp_);
        ImGuiHelper::OnDrawInputField("waxSeal_", waxSeal_);
        ImGuiHelper::OnDrawInputField("restoredStampSprite_", restoredStampSprite_);
        ImGuiHelper::OnDrawInputField("lockedStampSprite_", lockedStampSprite_);
        ImGuiHelper::OnDrawInputField("selectedTicketSprite_", selectedTicketSprite_);
        ImGuiHelper::OnDrawInputField("unselectedTicketSprite_", unselectedTicketSprite_);
        ImGuiHelper::OnDrawInputField("hoverSound_", hoverSound_);
        ImGuiHelper::OnDrawInputField("selectedScale_", selectedScale_);
        ImGuiHelper::OnDrawInputField("defaultNoteColor_", defaultNoteColor_);
        ImGuiHelper::OnDrawInputField("refusedNoteColor_", refusedNoteColor_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardRestorationRow);
#pragma endregion
