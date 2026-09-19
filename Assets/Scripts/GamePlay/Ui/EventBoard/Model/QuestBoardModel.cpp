#include "QuestBoardModel.h"

#include <algorithm>

#include "EventBoardFormat.h"
#include "../../Format/Ui_MoneyFormat.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../../Core/Game/PlayerAvatar/Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"

namespace GamePlay::Ui
{
    namespace
    {
        QuestBoardState ResolveQuestBoardState(
            const Asset::BoardQuest& quest,
            const GameCore::PlayerAvatar::IQuestGroup* takingQuests,
            const GameCore::PlayerAvatar::Quest::ICompleteQuestGroup* completedQuests)
        {
            const auto& content = quest.Quest();
            if (!content)
                return QuestBoardState::Preparing;
            if (completedQuests && completedQuests->CheckCompleted(content->QuestType()))
                return QuestBoardState::Cleared;
            if (takingQuests && takingQuests->IsTaking(content->QuestType()))
                return QuestBoardState::Taking;
            return QuestBoardState::Open;
        }

        std::vector<QuestBoardEntry> BuildQuestBoardEntries(
            const std::vector<std::shared_ptr<Asset::BoardQuest>>& quests,
            const std::chrono::sys_seconds now,
            const GameCore::PlayerAvatar::IQuestGroup* takingQuests,
            const GameCore::PlayerAvatar::Quest::ICompleteQuestGroup* completedQuests)
        {
            std::vector<QuestBoardEntry> entries;
            for (const auto& quest : quests)
            {
                const auto event = quest->Event();
                if (event && !event->IsOngoing(now))
                    continue;

                QuestBoardEntry entry;
                entry.quest        = quest;
                entry.state        = ResolveQuestBoardState(*quest, takingQuests, completedQuests);
                entry.isEventQuest = event != nullptr;

                const auto stage = quest->Stage();
                entry.placeText  = stage ? stage->DisplayName() : "―";
                entry.rewardText = quest->Quest() ? FormatMoney(quest->Quest()->RewardMoney().Value()) : "―";

                const auto end = event ? event->EndTime() : std::nullopt;
                entry.limitText = end ? FormatEventBoardDateTime(*end) + " まで" : "なし";
                entry.stateText = ToQuestBoardStateText(entry.state);
                entries.push_back(std::move(entry));
            }

            std::stable_partition(entries.begin(), entries.end(), [](const QuestBoardEntry& entry)
            {
                return entry.state != QuestBoardState::Cleared;
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
        }
        return "";
    }

    QuestBoardModel::QuestBoardModel(
        const std::vector<std::shared_ptr<Asset::BoardQuest>>& quests,
        const std::chrono::sys_seconds now,
        const GameCore::PlayerAvatar::IQuestGroup* takingQuests,
        const GameCore::PlayerAvatar::Quest::ICompleteQuestGroup* completedQuests,
        const size_t visibleRowCount)
        : entries_(BuildQuestBoardEntries(quests, now, takingQuests, completedQuests))
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
}
