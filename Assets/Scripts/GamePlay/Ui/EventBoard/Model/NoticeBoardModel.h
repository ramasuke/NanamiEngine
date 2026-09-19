#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "AnnouncementReadLog.h"
#include "BoardListCursor.h"
#include "../../../../../../Engine/Module/Namespace/EngineNamespace.h"
#include "../../../../../Data/EventNotice/Data_Announcement.h"

namespace GamePlay::Ui
{
    /** @brief 掲示板に並べるお知らせ1件。表示用の文字列は開いた時刻で作っておく */
    struct NoticeBoardEntry
    {
        std::shared_ptr<Asset::Announcement> announcement;
        bool        isUnread = false;
        std::string dateText;
        std::string dateTimeText;
    };

    /**
     * お知らせの一覧のModel。掲載時刻を過ぎたものだけを新しい順に並べる。
     * 選ばれたお知らせはその場で既読にする(右に本文が出た時点で読んだとみなす)。
     */
    class NoticeBoardModel final
    {
    public:
        NoticeBoardModel(
            const std::vector<std::shared_ptr<Asset::Announcement>>& announcements,
            std::chrono::sys_seconds now,
            size_t visibleRowCount);

        [[nodiscard]] const std::vector<NoticeBoardEntry>& Entries() const { return entries_; }
        [[nodiscard]] const NoticeBoardEntry* Selected() const;
        [[nodiscard]] BoardListCursor&       Cursor()       { return cursor_; }
        [[nodiscard]] const BoardListCursor& Cursor() const { return cursor_; }
        [[nodiscard]] size_t UnreadCount() const;

        /** @return 新しく既読にしたら true */
        bool MarkSelectedRead();

    private:
        AnnouncementReadLog readLog_;
        std::vector<NoticeBoardEntry> entries_;
        BoardListCursor cursor_;
    };
}
