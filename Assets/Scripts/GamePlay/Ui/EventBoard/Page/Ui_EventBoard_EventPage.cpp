#include "Ui_EventBoard_EventPage.h"

#include "../Row/EventBoardRowPool.h"

namespace GamePlay::Ui
{
    void EventBoardEventPage::BuildRows(const size_t count)
    {
        if (!rows_.empty())
            return;

        rows_ = InstantiateEventBoardRows<EventBoardRow>(rowPrefab_, rowsRoot_, count, rowSpacing_px_);
    }

    void EventBoardEventPage::SubscribeOnClickRow(std::function<void(size_t)> onClick) const
    {
        SubscribeOnClickEventBoardRows(rows_, std::move(onClick));
    }

    void EventBoardEventPage::Bind(const EventBoardModel& model) const
    {
        BindEventBoardRows(rows_, model.Entries(), model.Cursor(), moreAboveMark_, moreBelowMark_);
        ShowDetail(model.Selected());
    }

    void EventBoardEventPage::ShowDetail(const EventBoardEntry* entry) const
    {
        if (const auto root = detailRoot_.get())
            root->SetEnable(entry != nullptr);
        if (const auto empty = emptyText_.get())
            empty->SetEnable(entry == nullptr);
        if (!entry)
            return;

        const auto& notice = *entry->notice;
        if (const auto banner = detailBanner_.get())
        {
            const auto sprite = notice.BannerSprite();
            banner->SetEnable(sprite != nullptr);
            if (sprite)
                banner->SetSprite(sprite);
        }
        detailTitleText_ ->SetText(notice.Title());
        detailTagText_   ->SetText(notice.TagText());
        detailPeriodText_->SetText(entry->periodText);
        detailStatusText_->SetText(entry->statusText);
        detailStatusText_->SetTextColor(entry->isOngoing ? ongoingStatusColor_ : upcomingStatusColor_);
        if (const auto stamp = detailOngoingStamp_.get())
            stamp->SetEnable(entry->isOngoing);

        const auto& lines = notice.DescriptionLines();
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
    }

    void EventBoardEventPage::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rowPrefab_", rowPrefab_);
        ImGuiHelper::OnDrawInputField("rowsRoot_", rowsRoot_);
        ImGuiHelper::OnDrawInputField("rowSpacing_px_", rowSpacing_px_);
        ImGuiHelper::OnDrawInputField("maxVisibleRows_", maxVisibleRows_);
        ImGuiHelper::OnDrawInputField("moreAboveMark_", moreAboveMark_);
        ImGuiHelper::OnDrawInputField("moreBelowMark_", moreBelowMark_);
        ImGuiHelper::OnDrawInputField("detailRoot_", detailRoot_);
        ImGuiHelper::OnDrawInputField("detailBanner_", detailBanner_);
        ImGuiHelper::OnDrawInputField("detailTitleText_", detailTitleText_);
        ImGuiHelper::OnDrawInputField("detailTagText_", detailTagText_);
        ImGuiHelper::OnDrawInputField("detailPeriodText_", detailPeriodText_);
        ImGuiHelper::OnDrawInputField("detailStatusText_", detailStatusText_);
        ImGuiHelper::OnDrawInputField("detailOngoingStamp_", detailOngoingStamp_);
        ImGuiHelper::OnDrawInputField("detailDescriptionLines_", detailDescriptionLines_, [this]
        {
            if (ImGui::Button("Add Line"))
            {
                detailDescriptionLines_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("emptyText_", emptyText_);
        ImGuiHelper::OnDrawInputField("ongoingStatusColor_", ongoingStatusColor_);
        ImGuiHelper::OnDrawInputField("upcomingStatusColor_", upcomingStatusColor_);
    }
}
