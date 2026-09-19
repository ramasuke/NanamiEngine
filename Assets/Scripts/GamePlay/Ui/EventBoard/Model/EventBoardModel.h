#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "BoardListCursor.h"
#include "../../../../../../Engine/Module/Namespace/EngineNamespace.h"
#include "../../../../../Data/EventNotice/Data_EventNotice.h"

namespace GamePlay::Ui
{
    /** @brief 掲示板に並べる催し1件。表示用の文字列は開いた時刻で作っておく */
    struct EventBoardEntry
    {
        std::shared_ptr<Asset::EventNotice> notice;
        bool        isOngoing = false;
        std::string periodText;
        std::string statusText;
    };

    /**
     * 催しの一覧のModel。開いた時刻で終わった告知と書式の崩れた告知を落とし、開催中→開催予定の順に並べる。
     */
    class EventBoardModel final
    {
    public:
        EventBoardModel(
            const std::vector<std::shared_ptr<Asset::EventNotice>>& notices,
            std::chrono::sys_seconds now,
            size_t visibleRowCount);

        [[nodiscard]] const std::vector<EventBoardEntry>& Entries() const { return entries_; }
        [[nodiscard]] const EventBoardEntry* Selected() const;
        [[nodiscard]] BoardListCursor&       Cursor()       { return cursor_; }
        [[nodiscard]] const BoardListCursor& Cursor() const { return cursor_; }

    private:
        std::vector<EventBoardEntry> entries_;
        BoardListCursor cursor_;
    };
}
