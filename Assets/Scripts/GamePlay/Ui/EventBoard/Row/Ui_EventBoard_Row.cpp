#include "Ui_EventBoard_Row.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Sound/SoundPlayer.h"

namespace GamePlay::Ui
{
    void EventBoardRow::EnsureComponents()
    {
        if (selectButton_ && noticeRenderer_)
            return;

        selectButton_   = RequireComponent<NanamiUi::Button>();
        noticeRenderer_ = RequireComponent<Component::ImageRenderer>();
        baseScale_      = Transform().GetLocalScale();
    }

    void EventBoardRow::OnAwake()
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

    void EventBoardRow::Bind(const EventBoardEntry& entry)
    {
        EnsureComponents();

        titleText_ ->SetText(entry.notice->Title());
        statusText_->SetText(entry.statusText);
        statusText_->SetTextColor(entry.isOngoing ? ongoingStatusColor_ : upcomingStatusColor_);
        ongoingStamp_->SetEnable(entry.isOngoing);
    }

    void EventBoardRow::SubscribeOnClickSelectButton(std::function<void()> onClick)
    {
        EnsureComponents();
        selectButton_->OnClick().Subscribe([onClick](NanamiUi::MouseState)
        {
            onClick();
        }).AddTo(this);
    }

    void EventBoardRow::SetHighlighted(const bool isHighlighted)
    {
        isHighlighted_ = isHighlighted;
        RefreshAppearance();
    }

    void EventBoardRow::RefreshAppearance() const
    {
        noticeRenderer_->SetSprite(isHighlighted_ || isHovering_ ? selectedNoticeSprite_.get() : unselectedNoticeSprite_.get());
        waxSeal_->SetEnable(isHighlighted_);
        Transform().SetLocalScale(isHighlighted_ ? baseScale_ * selectedScale_ : baseScale_);
    }

    void EventBoardRow::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("titleText_", titleText_);
        ImGuiHelper::OnDrawInputField("statusText_", statusText_);
        ImGuiHelper::OnDrawInputField("ongoingStamp_", ongoingStamp_);
        ImGuiHelper::OnDrawInputField("waxSeal_", waxSeal_);
        ImGuiHelper::OnDrawInputField("selectedNoticeSprite_", selectedNoticeSprite_);
        ImGuiHelper::OnDrawInputField("unselectedNoticeSprite_", unselectedNoticeSprite_);
        ImGuiHelper::OnDrawInputField("hoverSound_", hoverSound_);
        ImGuiHelper::OnDrawInputField("selectedScale_", selectedScale_);
        ImGuiHelper::OnDrawInputField("ongoingStatusColor_", ongoingStatusColor_);
        ImGuiHelper::OnDrawInputField("upcomingStatusColor_", upcomingStatusColor_);
    }
}
