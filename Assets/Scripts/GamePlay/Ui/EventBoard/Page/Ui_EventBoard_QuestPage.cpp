#include "Ui_EventBoard_QuestPage.h"

#include <algorithm>

#include "Engine/Core/Platform/Draw2D/Draw2D.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void EventBoardQuestPage::BuildRows(const size_t count)
    {
        rows_.Build(rowPrefab_, rowsRoot_, count, rowSpacing_px_);
    }

    void EventBoardQuestPage::SubscribeOnClickRow(std::function<void(size_t)> onClick) const
    {
        rows_.SubscribeOnClick(std::move(onClick));
    }

    void EventBoardQuestPage::Bind(const QuestBoardModel& model) const
    {
        rows_.Bind(model.Entries(), model.Cursor(), moreAboveMark_, moreBelowMark_);
        ShowDetail(model.Selected());
    }

    void EventBoardQuestPage::FitPhotoToFrame(const int photoHandle) const
    {
        const auto size = Platform::Draw2D::GraphSize(photoHandle);
        if (!size || size->x <= 0 || size->y <= 0)
            return;
        const int width = size->x, height = size->y;

        // NOTE: 枠からはみ出す分は枠の縁 (10px) が隠すので、隙間が出ないよう大きい方の倍率で覆う
        const float scale = std::max(detailPhotoSize_px_.x / static_cast<float>(width),
                                     detailPhotoSize_px_.y / static_cast<float>(height));
        detailPhoto_->Transform().SetLocalScale(glm::vec3(scale, scale, 1.0f));
    }

    void EventBoardQuestPage::ShowDetail(const QuestBoardEntry* entry) const
    {
        if (const auto root = detailRoot_.get())
            root->SetEnable(entry != nullptr);
        if (const auto empty = emptyText_.get())
            empty->SetEnable(entry == nullptr);
        if (!entry)
            return;

        const auto& quest = *entry->quest;
        const auto stage  = quest.Stage();
        const auto photo  = stage ? stage->ThumbnailSprite() : nullptr;
        if (const auto photoRoot = detailPhotoRoot_.get())
            photoRoot->SetEnable(photo != nullptr);
        if (photo)
        {
            detailPhoto_->SetSprite(photo);
            FitPhotoToFrame(photo->GetDxLibHandle());
        }

        const auto event = quest.Event();
        detailEventChip_->SetEnable(event != nullptr);
        detailEventText_->SetEnable(event != nullptr);
        if (event)
            detailEventText_->SetText(event->Title());

        detailTitleText_ ->SetText(entry->titleText);
        detailClientText_->SetText(quest.ClientName());
        detailPlaceText_ ->SetText(entry->placeText);
        ShowQuestBoardRankPips(detailRankPips_, quest.Rank(), filledPipSprite_.get(), emptyPipSprite_.get());
        detailStateText_ ->SetText(entry->stateText);
        detailStateText_ ->SetTextColor(entry->state == QuestBoardState::Taking ? takingStateColor_ : defaultStateColor_);
        detailGoalText_  ->SetText(entry->goalText);
        detailRewardText_->SetText(entry->rewardText);
        detailLimitText_ ->SetText(entry->limitText);

        const auto& lines = quest.DescriptionLines();
        for (size_t i = 0; i < detailDescriptionLines_.size(); ++i)
        {
            const auto text = detailDescriptionLines_[i].get();
            if (!text)
                continue;

            const bool hasLine = entry->state != QuestBoardState::Locked && i < lines.size();
            text->SetEnable(hasLine);
            if (hasLine)
                text->SetText(lines[i]);
        }

        const QuestBoardStampSprites stamps{ takingSealSprite_.get(), clearedSealSprite_.get(), preparingSealSprite_.get(), lockedSealSprite_.get() };
        const auto seal = stamps.For(entry->state);
        detailSeal_->SetSprite(seal ? seal : openSealSprite_.get());
    }

    void EventBoardQuestPage::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rowPrefab_", rowPrefab_);
        ImGuiHelper::OnDrawInputField("rowsRoot_", rowsRoot_);
        ImGuiHelper::OnDrawInputField("rowSpacing_px_", rowSpacing_px_);
        ImGuiHelper::OnDrawInputField("maxVisibleRows_", maxVisibleRows_);
        ImGuiHelper::OnDrawInputField("moreAboveMark_", moreAboveMark_);
        ImGuiHelper::OnDrawInputField("moreBelowMark_", moreBelowMark_);
        ImGuiHelper::OnDrawInputField("detailRoot_", detailRoot_);
        ImGuiHelper::OnDrawInputField("detailPhotoRoot_", detailPhotoRoot_);
        ImGuiHelper::OnDrawInputField("detailPhoto_", detailPhoto_);
        ImGuiHelper::OnDrawInputField("detailEventChip_", detailEventChip_);
        ImGuiHelper::OnDrawInputField("detailEventText_", detailEventText_);
        ImGuiHelper::OnDrawInputField("detailTitleText_", detailTitleText_);
        ImGuiHelper::OnDrawInputField("detailClientText_", detailClientText_);
        ImGuiHelper::OnDrawInputField("detailPlaceText_", detailPlaceText_);
        ImGuiHelper::OnDrawInputField("detailRankPips_", detailRankPips_, [this]
        {
            if (ImGui::Button("Add Pip"))
            {
                detailRankPips_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("detailStateText_", detailStateText_);
        ImGuiHelper::OnDrawInputField("detailGoalText_", detailGoalText_);
        ImGuiHelper::OnDrawInputField("detailRewardText_", detailRewardText_);
        ImGuiHelper::OnDrawInputField("detailLimitText_", detailLimitText_);
        ImGuiHelper::OnDrawInputField("detailDescriptionLines_", detailDescriptionLines_, [this]
        {
            if (ImGui::Button("Add Line"))
            {
                detailDescriptionLines_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("detailSeal_", detailSeal_);
        ImGuiHelper::OnDrawInputField("emptyText_", emptyText_);
        ImGuiHelper::OnDrawInputField("filledPipSprite_", filledPipSprite_);
        ImGuiHelper::OnDrawInputField("emptyPipSprite_", emptyPipSprite_);
        ImGuiHelper::OnDrawInputField("openSealSprite_", openSealSprite_);
        ImGuiHelper::OnDrawInputField("takingSealSprite_", takingSealSprite_);
        ImGuiHelper::OnDrawInputField("clearedSealSprite_", clearedSealSprite_);
        ImGuiHelper::OnDrawInputField("preparingSealSprite_", preparingSealSprite_);
        ImGuiHelper::OnDrawInputField("lockedSealSprite_", lockedSealSprite_);
        ImGui::InputFloat2("detailPhotoSize_px_", &detailPhotoSize_px_.x);
        ImGuiHelper::OnDrawInputField("takingStateColor_", takingStateColor_);
        ImGuiHelper::OnDrawInputField("defaultStateColor_", defaultStateColor_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardQuestPage);
#pragma endregion
