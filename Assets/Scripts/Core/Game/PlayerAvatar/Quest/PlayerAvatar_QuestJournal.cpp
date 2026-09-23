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
    }

    void QuestJournal::Save() const
    {
        LocalPrefs::Save(TAKING_QUEST_SAVE_KEY, takingQuests_);
        completedQuests_.Save();
    }

    bool QuestJournal::Take(const std::shared_ptr<ITakeableQuest>& quest)
    {
        return takingQuests_.Add(quest, Context());
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

    void QuestJournal::CompleteQuest(const QuestType& completeQuest)
    {
        const auto* quest = takingQuests_.Find(completeQuest);
        if (!quest)
            return;

        // 依頼は何度でも受けられるので、達成のたびに報酬を出し、達成済みとしては残さない。
        // メインストーリーは職業をまたいで初回だけ報酬を出す
        const auto reward      = quest->RewardMoney();
        const bool rewardsNow  = quest->IsRepeatable() || MarkCompleted(completeQuest);
        takingQuests_.Remove(completeQuest);

        if (rewardsNow)
            onRewarded_.OnNext(reward);
    }

    bool QuestJournal::CheckCompleted(const QuestType& quest) const
    {
        return completedQuests_.CheckCompleted(quest);
    }

    bool QuestJournal::MarkCompleted(const QuestType& quest)
    {
        if (completedQuests_.CheckCompleted(quest))
            return false;

        completedQuests_.Subscribe(quest);
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
