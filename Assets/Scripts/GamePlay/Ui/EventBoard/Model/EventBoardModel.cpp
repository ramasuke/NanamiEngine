#include "EventBoardModel.h"

#include <algorithm>

#include "EventBoardFormat.h"

namespace GamePlay::Ui
{
    namespace
    {
        std::string MakeEventBoardStatusText(
            const bool isOngoing,
            const std::chrono::sys_seconds start,
            const std::chrono::sys_seconds end,
            const std::chrono::sys_seconds now)
        {
            if (isOngoing)
            {
                const auto remainingDays = std::chrono::floor<std::chrono::days>(end - now).count();
                return remainingDays >= 1 ? "開催中・残り" + std::to_string(remainingDays) + "日" : "開催中・まもなく終了";
            }

            const auto untilDays = std::chrono::floor<std::chrono::days>(start - now).count();
            return untilDays >= 1 ? "あと" + std::to_string(untilDays) + "日で開始" : "まもなく開始";
        }

        std::vector<EventBoardEntry> BuildEventBoardEntries(
            const std::vector<std::shared_ptr<Asset::EventNotice>>& notices,
            const std::chrono::sys_seconds now)
        {
            struct Sortable
            {
                EventBoardEntry entry;
                std::chrono::sys_seconds start;
            };
            std::vector<Sortable> sortables;

            for (const auto& notice : notices)
            {
                const auto start = notice->StartTime();
                const auto end   = notice->EndTime();
                if (!start || !end || *end <= *start || *end <= now)
                    continue;

                const bool isOngoing = *start <= now;
                sortables.push_back({
                    EventBoardEntry{
                        notice,
                        isOngoing,
                        FormatEventBoardDateTime(*start) + " 〜 " + FormatEventBoardDateTime(*end),
                        MakeEventBoardStatusText(isOngoing, *start, *end, now) },
                    *start });
            }

            std::stable_sort(sortables.begin(), sortables.end(), [](const Sortable& a, const Sortable& b)
            {
                if (a.entry.isOngoing != b.entry.isOngoing)
                    return a.entry.isOngoing;
                return a.start < b.start;
            });

            std::vector<EventBoardEntry> entries;
            for (auto& sortable : sortables)
                entries.push_back(std::move(sortable.entry));
            return entries;
        }
    }

    EventBoardModel::EventBoardModel(
        const std::vector<std::shared_ptr<Asset::EventNotice>>& notices,
        const std::chrono::sys_seconds now,
        const size_t visibleRowCount)
        : entries_(BuildEventBoardEntries(notices, now))
        , cursor_(entries_.size(), visibleRowCount)
    {
    }

    const EventBoardEntry* EventBoardModel::Selected() const
    {
        if (cursor_.SelectedIndex() >= entries_.size())
            return nullptr;

        return &entries_[cursor_.SelectedIndex()];
    }
}
