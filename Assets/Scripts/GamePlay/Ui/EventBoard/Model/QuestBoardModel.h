#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "BoardListCursor.h"
#include "QuestReadLog.h"
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

namespace GameCore::Story
{
    class StoryProgress;
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
        /** 解放条件を満たしていない依頼。「？？？」で貼り出し、受けられない */
        Locked,
    };

    /** @brief 掲示板に並べる依頼1件。表示用の文字列は開いた時刻で作っておく */
    struct QuestBoardEntry
    {
        std::shared_ptr<Asset::BoardQuest> quest;
        QuestBoardState state = QuestBoardState::Open;
        bool        isEventQuest = false;
        /** 未解放なら「？？？」 */
        std::string titleText;
        /** 未解放なら解放条件の文言 */
        std::string goalText;
        std::string placeText;
        std::string rewardText;
        std::string limitText;
        std::string stateText;
    };

    /**
     * 依頼の一覧のModel。期間外のイベント依頼を落とし、未解放・達成済みの順で末尾へ回す(ほかはデータの順)。
     * 受注中・達成・解放の判定は開いた時点のプレイヤーのクエスト記録と物語の進み具合から読む。
     */
    class QuestBoardModel final
    {
    public:
        /** @param takingQuests / completedQuests プレイヤーがいなければ nullptr(解放条件も見ず、全部受付中として出す) */
        QuestBoardModel(
            const std::vector<std::shared_ptr<Asset::BoardQuest>>& quests,
            std::chrono::sys_seconds now,
            const GameCore::PlayerAvatar::IQuestGroup* takingQuests,
            const GameCore::PlayerAvatar::Quest::ICompleteQuestGroup* completedQuests,
            const GameCore::Story::StoryProgress* story,
            size_t visibleRowCount);

        [[nodiscard]] const std::vector<QuestBoardEntry>& Entries() const { return entries_; }
        [[nodiscard]] const QuestBoardEntry* Selected() const;
        [[nodiscard]] BoardListCursor&       Cursor()       { return cursor_; }
        [[nodiscard]] const BoardListCursor& Cursor() const { return cursor_; }

        /** @brief 選んでいる依頼を受けたことにする。受付中でなければ何もしない */
        void MarkSelectedTaking();

        /** @brief 受付中のメインストーリーの依頼に、まだ掲示板で見ていないものがあるか */
        [[nodiscard]] bool HasUnreadMainStory(const QuestReadLog& readLog) const;
        /** @brief 受付中のメインストーリーの依頼をすべて見たことにする */
        void MarkMainStoryRead(QuestReadLog& readLog) const;

    private:
        std::vector<QuestBoardEntry> entries_;
        BoardListCursor cursor_;
    };

    /** @brief 状況欄の文言 */
    [[nodiscard]] std::string ToQuestBoardStateText(QuestBoardState state);
}
