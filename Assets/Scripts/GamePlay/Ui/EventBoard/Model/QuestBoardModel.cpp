#include "QuestBoardModel.h"

#include <algorithm>

#include "EventBoardFormat.h"
#include "../../Format/Ui_MoneyFormat.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_MainStoryQuestBase.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/Unlock/PlayerAvatar_QuestUnlockContext.h"

namespace GamePlay::Ui
{
    namespace
    {
        bool IsOpenMainStory(const QuestBoardEntry& entry)
        {
            return entry.state == QuestBoardState::Open
                && dynamic_cast<const GameCore::PlayerAvatar::MainStoryQuestBase*>(entry.quest->Quest().get()) != nullptr;
        }

        QuestBoardState ResolveQuestBoardState(
            const Asset::BoardQuest& quest,
            const GameCore::PlayerAvatar::IQuestGroup* takingQuests,
            const GameCore::PlayerAvatar::Quest::ICompleteQuestGroup* completedQuests,
            const GameCore::PlayerAvatar::Quest::Unlock::QuestUnlockContext& unlockContext)
        {
            const auto& content = quest.Quest();
            if (content && completedQuests && completedQuests->CheckCompleted(content->QuestType()))
                return QuestBoardState::Cleared;
            if (content && takingQuests && takingQuests->IsTaking(content->QuestType()))
                return QuestBoardState::Taking;
            if (completedQuests && !quest.IsUnlocked(unlockContext))
                return QuestBoardState::Locked;
            if (!content)
                return QuestBoardState::Preparing;
            return QuestBoardState::Open;
        }

        std::vector<QuestBoardEntry> BuildQuestBoardEntries(
            const std::vector<std::shared_ptr<Asset::BoardQuest>>& quests,
            const std::chrono::sys_seconds now,
            const GameCore::PlayerAvatar::IQuestGroup* takingQuests,
            const GameCore::PlayerAvatar::Quest::ICompleteQuestGroup* completedQuests,
            const GameCore::Story::StoryProgress* story)
        {
            const GameCore::PlayerAvatar::Quest::Unlock::QuestUnlockContext unlockContext{ story, completedQuests };

            std::vector<QuestBoardEntry> entries;
            for (const auto& quest : quests)
            {
                const auto event = quest->Event();
                if (event && !event->IsOngoing(now))
                    continue;

                QuestBoardEntry entry;
                entry.quest        = quest;
                entry.state        = ResolveQuestBoardState(*quest, takingQuests, completedQuests, unlockContext);
                entry.isEventQuest = event != nullptr;

                const auto stage = quest->Stage();
                entry.placeText  = stage ? stage->DisplayName() : "―";
                entry.rewardText = quest->Quest() ? FormatMoney(quest->Quest()->RewardMoney().Value()) : "―";

                const auto end = event ? event->EndTime() : std::nullopt;
                entry.limitText = end ? FormatEventBoardDateTime(*end) + " まで" : "なし";
                entry.stateText = ToQuestBoardStateText(entry.state);
                entry.titleText = quest->Title();
                entry.goalText  = quest->GoalText();
                if (entry.state == QuestBoardState::Locked)
                {
                    entry.titleText  = "？？？";
                    entry.goalText   = quest->LockedText();
                    entry.rewardText = "―";
                    entry.limitText  = "―";
                }
                entries.push_back(std::move(entry));
            }

            const auto lockedBegin = std::stable_partition(entries.begin(), entries.end(), [](const QuestBoardEntry& entry)
            {
                return entry.state != QuestBoardState::Locked && entry.state != QuestBoardState::Cleared;
            });
            std::stable_partition(lockedBegin, entries.end(), [](const QuestBoardEntry& entry)
            {
                return entry.state == QuestBoardState::Locked;
            });
            return entries;
        }
    }

    std::string ToQuestBoardStateText(const QuestBoardState state)
    {
        switch (state)
        {
        case QuestBoardState::Open:      return "受付中";
        case QuestBoardState::Taking:    return "受注中";
        case QuestBoardState::Cleared:   return "達成済み";
        case QuestBoardState::Preparing: return "準備中";
        case QuestBoardState::Locked:    return "未解放";
        }
        return "";
    }

    QuestBoardModel::QuestBoardModel(
        const std::vector<std::shared_ptr<Asset::BoardQuest>>& quests,
        const std::chrono::sys_seconds now,
        const GameCore::PlayerAvatar::IQuestGroup* takingQuests,
        const GameCore::PlayerAvatar::Quest::ICompleteQuestGroup* completedQuests,
        const GameCore::Story::StoryProgress* story,
        const size_t visibleRowCount)
        : entries_(BuildQuestBoardEntries(quests, now, takingQuests, completedQuests, story))
        , cursor_(entries_.size(), visibleRowCount)
    {
    }

    const QuestBoardEntry* QuestBoardModel::Selected() const
    {
        if (cursor_.SelectedIndex() >= entries_.size())
            return nullptr;

        return &entries_[cursor_.SelectedIndex()];
    }

    void QuestBoardModel::MarkSelectedTaking()
    {
        if (cursor_.SelectedIndex() >= entries_.size())
            return;

        auto& entry = entries_[cursor_.SelectedIndex()];
        if (entry.state != QuestBoardState::Open)
            return;

        entry.state     = QuestBoardState::Taking;
        entry.stateText = ToQuestBoardStateText(entry.state);
    }

    bool QuestBoardModel::HasUnreadMainStory(const QuestReadLog& readLog) const
    {
        return std::ranges::any_of(entries_, [&readLog](const QuestBoardEntry& entry)
        {
            return IsOpenMainStory(entry) && !readLog.IsRead(entry.quest->GetGuid().Value());
        });
    }

    void QuestBoardModel::MarkMainStoryRead(QuestReadLog& readLog) const
    {
        for (const auto& entry : entries_)
        {
            if (IsOpenMainStory(entry))
                readLog.MarkRead(entry.quest->GetGuid().Value());
        }
    }
}
