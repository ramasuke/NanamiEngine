#include "PlayerAvatar_QuestJournal.h"

#include "PlayerAvatar_ITakeableQuest.h"
#include "../Record/PlayerAvatar_RecordBook.h"
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore::PlayerAvatar::Quest
{
    namespace
    {
        constexpr auto TAKING_QUEST_SAVE_KEY = "TakingQuests";
    }

    QuestJournal::QuestJournal()
    {
        Reload();
    }

    QuestJournal::~QuestJournal() = default;

    void QuestJournal::Reload()
    {
        takingQuests_ = LocalPrefs::LoadOrDefault<QuestList>(TAKING_QUEST_SAVE_KEY, QuestList());
        completedQuests_.Reload();
        takingQuests_.StartAll(Context());
        onChanged_.OnNext(NanamiEngine::R4::Unit{});
    }

    void QuestJournal::Save() const
    {
        LocalPrefs::Save(TAKING_QUEST_SAVE_KEY, takingQuests_);
        completedQuests_.Save();
    }

    bool QuestJournal::Take(const std::shared_ptr<ITakeableQuest>& quest)
    {
        if (!takingQuests_.Add(quest, Context()))
            return false;

        onChanged_.OnNext(NanamiEngine::R4::Unit{});
        return true;
    }

    void QuestJournal::Adopt(const std::vector<std::shared_ptr<ITakeableQuest>>& quests)
    {
        for (const auto& quest : quests)
            Take(quest);
    }

    bool QuestJournal::IsTaking(const QuestType& quest) const
    {
        return takingQuests_.Contains(quest);
    }

    bool QuestJournal::IsTakingBoardQuest(const std::string& boardQuestGuid, const QuestType& type) const
    {
        return takingQuests_.ContainsBoardQuest(boardQuestGuid, type);
    }

    void QuestJournal::CompleteQuest(const QuestType& completeQuest)
    {
        if (const auto* quest = takingQuests_.Find(completeQuest))
            CompleteTakenQuest(*quest);
    }

    void QuestJournal::CompleteTakenQuest(const ITakeableQuest& quest)
    {
        // 繰り返せる依頼は達成のたびに報酬。それ以外は初回だけで、メインストーリーは職業をまたいで QuestType で残す
        const auto rewards    = quest.Rewards();
        const auto& guid      = quest.BoardQuestGuid();
        const bool byType     = quest.RecordsCompletionByType() || guid.empty();
        const bool rewardsNow = quest.IsRepeatable()
            || (byType ? MarkCompleted(quest.QuestType()) : MarkBoardQuestCompleted(guid));
        // WARNING: Remove で quest が破棄されるので、以降 quest に触れない
        takingQuests_.Remove(&quest);
        onChanged_.OnNext(NanamiEngine::R4::Unit{});

        if (rewardsNow)
            onRewarded_.OnNext(rewards);
    }

    bool QuestJournal::CheckCompleted(const QuestType& quest) const
    {
        return completedQuests_.CheckCompleted(quest);
    }

    bool QuestJournal::IsBoardQuestCompleted(const std::string& boardQuestGuid) const
    {
        return completedQuests_.CheckBoardQuestCompleted(boardQuestGuid);
    }

    bool QuestJournal::MarkCompleted(const QuestType& quest)
    {
        if (completedQuests_.CheckCompleted(quest))
            return false;

        completedQuests_.Subscribe(quest);
        onChanged_.OnNext(NanamiEngine::R4::Unit{});
        return true;
    }

    bool QuestJournal::MarkBoardQuestCompleted(const std::string& boardQuestGuid)
    {
        if (completedQuests_.CheckBoardQuestCompleted(boardQuestGuid))
            return false;

        completedQuests_.SubscribeBoardQuest(boardQuestGuid);
        onChanged_.OnNext(NanamiEngine::R4::Unit{});
        return true;
    }

    void QuestJournal::OnDrawGui() const
    {
        if (!ImGui::CollapsingHeader("QuestJournal"))
            return;

        takingQuests_.OnDrawGui();
    }

    QuestContext QuestJournal::Context()
    {
        return QuestContext{ *this, Record::RecordBook::Instance() };
    }
}
