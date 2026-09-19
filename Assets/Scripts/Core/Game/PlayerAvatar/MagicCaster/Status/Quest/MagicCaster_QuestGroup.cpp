#include "MagicCaster_QuestGroup.h"

#include "../../../Quest/PlayerAvatar_QuestContext.h"
#include "../../../Quest/Completed/PlayerAvatar_CompletedQuestGroup.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    QuestGroup::QuestGroup()
        : completedQuests_(std::make_unique<Quest::CompletedQuestGroup>())
    {
    }

    QuestGroup::~QuestGroup() = default;

    void QuestGroup::Init(const std::shared_ptr<IStatusEvent>& statusEvent,
                          const std::shared_ptr<Wallet>& wallet)
    {
        statusEvent_ = statusEvent;
        wallet_      = wallet;

        storyQuests_.StartAll(Quest::QuestContext{ *statusEvent_, *this });
    }

    void QuestGroup::Subscribe(const std::shared_ptr<StoryQuestBase>& addQuest)
    {
        storyQuests_.Add(addQuest, Quest::QuestContext{ *statusEvent_, *this });
    }

    bool QuestGroup::IsTaking(const QuestType& quest) const
    {
        return storyQuests_.Contains(quest);
    }

    void QuestGroup::OnDrawGui()
    {
        storyQuests_.OnDrawGui();
    }

    void QuestGroup::CompleteQuest(const QuestType& completeQuest)
    {
        // 完了フラグは剣士と同じ保存先に残るので、報酬が出るのは職業をまたいで初回だけ
        if (wallet_ && !completedQuests_->CheckCompleted(completeQuest))
        {
            if (const auto reward = storyQuests_.RewardOf(completeQuest))
                wallet_->Earn(*reward);
        }

        completedQuests_->Subscribe(completeQuest);
        storyQuests_.Remove(completeQuest);
    }

    bool QuestGroup::CheckCompleted(const QuestType& quest) const
    {
        return completedQuests_->CheckCompleted(quest);
    }
}
