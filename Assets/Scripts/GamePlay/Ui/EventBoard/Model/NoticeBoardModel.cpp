#include "NoticeBoardModel.h"

#include <algorithm>

#include "EventBoardFormat.h"

namespace GamePlay::Ui
{
    namespace
    {
        std::vector<NoticeBoardEntry> BuildNoticeBoardEntries(
            const std::vector<std::shared_ptr<Asset::Announcement>>& announcements,
            const std::chrono::sys_seconds now,
            const AnnouncementReadLog& readLog)
        {
            struct Sortable
            {
                NoticeBoardEntry entry;
                std::chrono::sys_seconds postedAt;
            };
            std::vector<Sortable> sortables;

            for (const auto& announcement : announcements)
            {
                const auto postedAt = announcement->PostedTime();
                if (!postedAt || now < *postedAt)
                    continue;

                sortables.push_back({
                    NoticeBoardEntry{
                        announcement,
                        !readLog.IsRead(announcement->GetGuid().Value()),
                        FormatEventBoardDate(*postedAt),
                        FormatEventBoardDateTime(*postedAt) },
                    *postedAt });
            }

            std::stable_sort(sortables.begin(), sortables.end(), [](const Sortable& a, const Sortable& b)
            {
                return a.postedAt > b.postedAt;
            });

            std::vector<NoticeBoardEntry> entries;
            for (auto& sortable : sortables)
                entries.push_back(std::move(sortable.entry));
            return entries;
        }
    }

    NoticeBoardModel::NoticeBoardModel(
        const std::vector<std::shared_ptr<Asset::Announcement>>& announcements,
        const std::chrono::sys_seconds now,
        const size_t visibleRowCount)
        : entries_(BuildNoticeBoardEntries(announcements, now, readLog_))
        , cursor_(entries_.size(), visibleRowCount)
    {
    }

    const NoticeBoardEntry* NoticeBoardModel::Selected() const
    {
        if (cursor_.SelectedIndex() >= entries_.size())
            return nullptr;

        return &entries_[cursor_.SelectedIndex()];
    }

    size_t NoticeBoardModel::UnreadCount() const
    {
        return static_cast<size_t>(std::ranges::count_if(entries_, [](const NoticeBoardEntry& entry)
        {
            return entry.isUnread;
        }));
    }

    bool NoticeBoardModel::MarkSelectedRead()
    {
        if (cursor_.SelectedIndex() >= entries_.size())
            return false;

        auto& entry = entries_[cursor_.SelectedIndex()];
        if (!entry.isUnread)
            return false;

        entry.isUnread = false;
        readLog_.MarkRead(entry.announcement->GetGuid().Value());
        return true;
    }
}
