#include "Ui_EventBoard_RestorationPage.h"

#include "../../Format/Ui_MoneyFormat.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void EventBoardRestorationPage::BuildRows(const size_t count)
    {
        rows_.Build(rowPrefab_, rowsRoot_, count, rowSpacing_px_);
    }

    void EventBoardRestorationPage::SubscribeOnClickRow(std::function<void(size_t)> onClick) const
    {
        rows_.SubscribeOnClick(std::move(onClick));
    }

    void EventBoardRestorationPage::Bind(const RestorationBoardModel& model) const
    {
        rows_.Bind(model.Entries(), model.Cursor(), moreAboveMark_, moreBelowMark_);
        ShowDetail(model.Selected(), model.Balance());
    }

    void EventBoardRestorationPage::ShowDetail(const RestorationBoardEntry* entry, const int balance) const
    {
        if (const auto root = detailRoot_.get())
            root->SetEnable(entry != nullptr);
        if (const auto empty = emptyText_.get())
            empty->SetEnable(entry == nullptr);
        if (!entry)
            return;

        const auto& facility = *entry->facility;

        const bool isOpen    = entry->state == RestorationBoardState::Open;
        const bool isLocked  = entry->state == RestorationBoardState::Locked;
        const bool isRefused = isOpen && !entry->isAffordable;

        detailNameText_ ->SetText(facility.Name());
        detailStateText_->SetText(entry->stateText);
        detailStateText_->SetTextColor(isLocked ? fadedColor_ : isOpen && !isRefused ? defaultColor_ : refusedColor_);

        // NOTE: 満たした前提も薄く残す。何を済ませて開いたのかが分かる
        const auto& condition = facility.ConditionText();
        detailConditionText_->SetText(condition.empty() ? noConditionText_ : condition);
        detailConditionText_->SetTextColor(isLocked ? refusedColor_ : fadedColor_);

        detailCostText_   ->SetText(entry->costText);
        detailBalanceText_->SetText(FormatMoney(balance));
        detailRemainText_ ->SetText(isOpen ? FormatMoney(balance - facility.Cost()) : notApplicableText_);
        detailRemainText_ ->SetTextColor(!isOpen ? fadedColor_ : isRefused ? refusedColor_ : remainColor_);

        const auto& lines = facility.DescriptionLines();
        for (size_t i = 0; i < detailDescriptionLines_.size(); ++i)
        {
            const auto text = detailDescriptionLines_[i].get();
            if (!text)
                continue;

            const bool hasLine = i < lines.size();
            text->SetEnable(hasLine);
            if (hasLine)
                text->SetText(lines[i]);
        }

        const auto seal = entry->state == RestorationBoardState::Restored ? restoredSealSprite_.get()
                        : isLocked                                        ? lockedSealSprite_.get()
                        : openSealSprite_.get();
        detailSeal_->SetSprite(seal);
    }

    void EventBoardRestorationPage::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rowPrefab_", rowPrefab_);
        ImGuiHelper::OnDrawInputField("rowsRoot_", rowsRoot_);
        ImGuiHelper::OnDrawInputField("rowSpacing_px_", rowSpacing_px_);
        ImGuiHelper::OnDrawInputField("maxVisibleRows_", maxVisibleRows_);
        ImGuiHelper::OnDrawInputField("moreAboveMark_", moreAboveMark_);
        ImGuiHelper::OnDrawInputField("moreBelowMark_", moreBelowMark_);
        ImGuiHelper::OnDrawInputField("detailRoot_", detailRoot_);
        ImGuiHelper::OnDrawInputField("detailNameText_", detailNameText_);
        ImGuiHelper::OnDrawInputField("detailStateText_", detailStateText_);
        ImGuiHelper::OnDrawInputField("detailConditionText_", detailConditionText_);
        ImGuiHelper::OnDrawInputField("detailCostText_", detailCostText_);
        ImGuiHelper::OnDrawInputField("detailBalanceText_", detailBalanceText_);
        ImGuiHelper::OnDrawInputField("detailRemainText_", detailRemainText_);
        ImGuiHelper::OnDrawInputField("detailDescriptionLines_", detailDescriptionLines_, [this]
        {
            if (ImGui::Button("Add Line"))
            {
                detailDescriptionLines_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("detailSeal_", detailSeal_);
        ImGuiHelper::OnDrawInputField("emptyText_", emptyText_);
        ImGuiHelper::OnDrawInputField("openSealSprite_", openSealSprite_);
        ImGuiHelper::OnDrawInputField("lockedSealSprite_", lockedSealSprite_);
        ImGuiHelper::OnDrawInputField("restoredSealSprite_", restoredSealSprite_);
        ImGuiHelper::OnDrawInputField("defaultColor_", defaultColor_);
        ImGuiHelper::OnDrawInputField("fadedColor_", fadedColor_);
        ImGuiHelper::OnDrawInputField("refusedColor_", refusedColor_);
        ImGuiHelper::OnDrawInputField("remainColor_", remainColor_);
        ImGuiHelper::OnDrawInputField("notApplicableText_", notApplicableText_);
        ImGuiHelper::OnDrawInputField("noConditionText_", noConditionText_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardRestorationPage);
#pragma endregion
