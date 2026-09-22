#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "BoardListCursor.h"
#include "Engine/Module/Namespace/EngineNamespace.h"
#include "../../../../../Data/EventNotice/Data_BoardQuest.h"

namespace GameCore::PlayerAvatar
{
    class IQuestGroup;
}

namespace GameCore::PlayerAvatar::Quest
{
    class ICompleteQuestGroup;
}

namespace GamePlay::Ui
{
    enum class QuestBoardState
    {
        Open,
        Taking,
        Cleared,
        /** 中身のクエストがまだ付いていない依頼書。貼り出すが受けられない */
        Preparing,
    };

    /** @brief 掲示板に並べる依頼1件。表示用の文字列は開いた時刻で作っておく */
    struct QuestBoardEntry
    {
        std::shared_ptr<Asset::BoardQuest> quest;
        QuestBoardState state = QuestBoardState::Open;
        bool        isEventQuest = false;
        std::string placeText;
        std::string rewardText;
        std::string limitText;
        std::string stateText;
    };

    /**
     * 依頼の一覧のModel。期間外のイベント依頼を落とし、達成済みは末尾へ回す(ほかはデータの順)。
     * 受注中・達成の判定は開いた時点のプレイヤーのクエスト記録から読む。
     */
    class QuestBoardModel final
    {
    public:
        /** @param takingQuests / completedQuests プレイヤーがいなければ nullptr(全部受付中として出す) */
        QuestBoardModel(
            const std::vector<std::shared_ptr<Asset::BoardQuest>>& quests,
            std::chrono::sys_seconds now,
            const GameCore::PlayerAvatar::IQuestGroup* takingQuests,
            const GameCore::PlayerAvatar::Quest::ICompleteQuestGroup* completedQuests,
            size_t visibleRowCount);

        [[nodiscard]] const std::vector<QuestBoardEntry>& Entries() const { return entries_; }
        [[nodiscard]] const QuestBoardEntry* Selected() const;
        [[nodiscard]] BoardListCursor&       Cursor()       { return cursor_; }
        [[nodiscard]] const BoardListCursor& Cursor() const { return cursor_; }

        /** @brief 選んでいる依頼を受けたことにする。受付中でなければ何もしない */
        void MarkSelectedTaking();

    private:
        std::vector<QuestBoardEntry> entries_;
        BoardListCursor cursor_;
    };

    /** @brief 状況欄の文言 */
    [[nodiscard]] std::string ToQuestBoardStateText(QuestBoardState state);
}
