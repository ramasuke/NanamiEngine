#include "Ui_EventBoard_QuestRow.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Sound/SoundPlayer.h"

namespace GamePlay::Ui
{
    void ShowQuestBoardRankPips(
        const std::vector<FIELD(Component::ImageRenderer)>& pips,
        const int rank,
        const std::shared_ptr<Asset::SpriteFile>& filledSprite,
        const std::shared_ptr<Asset::SpriteFile>& emptySprite)
    {
        for (size_t i = 0; i < pips.size(); ++i)
        {
            if (const auto pip = pips[i].get())
                pip->SetSprite(static_cast<int>(i) < rank ? filledSprite : emptySprite);
        }
    }

    std::shared_ptr<Asset::SpriteFile> QuestBoardStampSprites::For(const QuestBoardState state) const
    {
        switch (state)
        {
        case QuestBoardState::Taking:    return taking;
        case QuestBoardState::Cleared:   return cleared;
        case QuestBoardState::Preparing: return preparing;
        case QuestBoardState::Open:      return nullptr;
        }
        return nullptr;
    }

    void EventBoardQuestRow::EnsureComponents()
    {
        if (selectButton_ && ticketRenderer_)
            return;

        selectButton_   = RequireComponent<NanamiUi::Button>();
        ticketRenderer_ = RequireComponent<Component::ImageRenderer>();
        baseScale_      = Transform().GetLocalScale();
    }

    void EventBoardQuestRow::OnAwake()
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

    void EventBoardQuestRow::Bind(const QuestBoardEntry& entry)
    {
        EnsureComponents();

        titleText_ ->SetText(entry.quest->Title());
        placeText_ ->SetText(entry.placeText);
        rewardText_->SetText(entry.rewardText);
        ShowQuestBoardRankPips(rankPips_, entry.quest->Rank(), filledPipSprite_.get(), emptyPipSprite_.get());
        eventChip_->SetEnable(entry.isEventQuest);

        const QuestBoardStampSprites stamps{ takingStampSprite_.get(), clearedStampSprite_.get(), preparingStampSprite_.get() };
        const auto stamp = stamps.For(entry.state);
        stateStamp_->SetEnable(stamp != nullptr);
        if (stamp)
            stateStamp_->SetSprite(stamp);
    }

    void EventBoardQuestRow::SubscribeOnClickSelectButton(std::function<void()> onClick)
    {
        EnsureComponents();
        selectButton_->OnClick().Subscribe([onClick](NanamiUi::MouseState)
        {
            onClick();
        }).AddTo(this);
    }

    void EventBoardQuestRow::SetHighlighted(const bool isHighlighted)
    {
        isHighlighted_ = isHighlighted;
        RefreshAppearance();
    }

    void EventBoardQuestRow::RefreshAppearance() const
    {
        ticketRenderer_->SetSprite(isHighlighted_ || isHovering_ ? selectedTicketSprite_.get() : unselectedTicketSprite_.get());
        waxSeal_->SetEnable(isHighlighted_);
        Transform().SetLocalScale(isHighlighted_ ? baseScale_ * selectedScale_ : baseScale_);
    }

    void EventBoardQuestRow::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("titleText_", titleText_);
        ImGuiHelper::OnDrawInputField("placeText_", placeText_);
        ImGuiHelper::OnDrawInputField("rewardText_", rewardText_);
        ImGuiHelper::OnDrawInputField("rankPips_", rankPips_, [this]
        {
            if (ImGui::Button("Add Pip"))
            {
                rankPips_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("eventChip_", eventChip_);
        ImGuiHelper::OnDrawInputField("stateStamp_", stateStamp_);
        ImGuiHelper::OnDrawInputField("waxSeal_", waxSeal_);
        ImGuiHelper::OnDrawInputField("filledPipSprite_", filledPipSprite_);
        ImGuiHelper::OnDrawInputField("emptyPipSprite_", emptyPipSprite_);
        ImGuiHelper::OnDrawInputField("takingStampSprite_", takingStampSprite_);
        ImGuiHelper::OnDrawInputField("clearedStampSprite_", clearedStampSprite_);
        ImGuiHelper::OnDrawInputField("preparingStampSprite_", preparingStampSprite_);
        ImGuiHelper::OnDrawInputField("selectedTicketSprite_", selectedTicketSprite_);
        ImGuiHelper::OnDrawInputField("unselectedTicketSprite_", unselectedTicketSprite_);
        ImGuiHelper::OnDrawInputField("hoverSound_", hoverSound_);
        ImGuiHelper::OnDrawInputField("selectedScale_", selectedScale_);
    }
}
