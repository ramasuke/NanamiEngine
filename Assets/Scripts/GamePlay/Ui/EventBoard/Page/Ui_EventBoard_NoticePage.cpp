#include "Ui_EventBoard_NoticePage.h"

#include "../Row/EventBoardRowPool.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void EventBoardNoticePage::BuildRows(const size_t count)
    {
        if (!rows_.empty())
            return;

        rows_ = InstantiateEventBoardRows<EventBoardNoticeRow>(rowPrefab_, rowsRoot_, count, rowSpacing_px_);
    }

    void EventBoardNoticePage::SubscribeOnClickRow(std::function<void(size_t)> onClick) const
    {
        SubscribeOnClickEventBoardRows(rows_, std::move(onClick));
    }

    void EventBoardNoticePage::Bind(const NoticeBoardModel& model) const
    {
        BindEventBoardRows(rows_, model.Entries(), model.Cursor(), moreAboveMark_, moreBelowMark_);
        ShowDetail(model.Selected());
    }

    void EventBoardNoticePage::ShowDetail(const NoticeBoardEntry* entry) const
    {
        if (const auto root = detailRoot_.get())
            root->SetEnable(entry != nullptr);
        if (const auto empty = emptyText_.get())
            empty->SetEnable(entry == nullptr);
        if (!entry)
            return;

        const auto& announcement = *entry->announcement;
        const auto hanko = FindAnnouncementKindSprite(kindHankoSprites_, announcement.Kind());
        detailKindHanko_->SetEnable(hanko != nullptr);
        if (hanko)
            detailKindHanko_->SetSprite(hanko);
        detailDateText_ ->SetText(entry->dateTimeText);
        detailTitleText_->SetText(announcement.Title());

        const auto& lines = announcement.BodyLines();
        for (size_t i = 0; i < detailBodyLines_.size(); ++i)
        {
            const auto text = detailBodyLines_[i].get();
            if (!text)
                continue;

            const bool hasLine = i < lines.size();
            text->SetEnable(hasLine);
            if (hasLine)
                text->SetText(lines[i]);
        }
    }

    void EventBoardNoticePage::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rowPrefab_", rowPrefab_);
        ImGuiHelper::OnDrawInputField("rowsRoot_", rowsRoot_);
        ImGuiHelper::OnDrawInputField("rowSpacing_px_", rowSpacing_px_);
        ImGuiHelper::OnDrawInputField("maxVisibleRows_", maxVisibleRows_);
        ImGuiHelper::OnDrawInputField("moreAboveMark_", moreAboveMark_);
        ImGuiHelper::OnDrawInputField("moreBelowMark_", moreBelowMark_);
        ImGuiHelper::OnDrawInputField("detailRoot_", detailRoot_);
        ImGuiHelper::OnDrawInputField("detailKindHanko_", detailKindHanko_);
        ImGuiHelper::OnDrawInputField("detailDateText_", detailDateText_);
        ImGuiHelper::OnDrawInputField("detailTitleText_", detailTitleText_);
        ImGuiHelper::OnDrawInputField("detailBodyLines_", detailBodyLines_, [this]
        {
            if (ImGui::Button("Add Line"))
            {
                detailBodyLines_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("emptyText_", emptyText_);
        ImGuiHelper::OnDrawInputField("kindHankoSprites_", kindHankoSprites_, [this]
        {
            if (ImGui::Button("Add Sprite"))
            {
                kindHankoSprites_.emplace_back();
            }
        });
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardNoticePage);
#pragma endregion
