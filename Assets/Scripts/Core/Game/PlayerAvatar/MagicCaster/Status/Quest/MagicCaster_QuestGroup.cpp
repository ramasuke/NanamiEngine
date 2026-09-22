#include "MagicCaster_QuestGroup.h"

#include "../../../Quest/PlayerAvatar_QuestContext.h"
#include "../../../Quest/Completed/PlayerAvatar_CompletedQuestGroup.h"
#include "../../../Record/PlayerAvatar_RecordBook.h"

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

        storyQuests_.StartAll(Quest::QuestContext{ *statusEvent_, *this, Record::RecordBook::Instance() });
    }

    void QuestGroup::Subscribe(const std::shared_ptr<Quest::ITakeableQuest>& addQuest)
    {
        storyQuests_.Add(addQuest, Quest::QuestContext{ *statusEvent_, *this, Record::RecordBook::Instance() });
    }

    bool QuestGroup::IsTaking(const QuestType& quest) const
    {
        return storyQuests_.Contains(quest);
    }

    void QuestGroup::OnDrawGui()
    {
        storyQuests_.OnDrawGui();
        Record::RecordBook::Instance().OnDrawGui();
    }

    void QuestGroup::CompleteQuest(const QuestType& completeQuest)
    {
        // 依頼は何度でも受けられるので、達成のたびに報酬を出し、達成済みとしては残さない
        if (const auto* request = storyQuests_.Find(completeQuest); request && request->IsRepeatable())
        {
            if (wallet_)
                wallet_->Earn(request->RewardMoney());
            storyQuests_.Remove(completeQuest);
            return;
        }

        // 完了フラグは剣士と同じ保存先に残るので、報酬が出るのは職業をまたいで初回だけ
        if (wallet_ && !completedQuests_->CheckCompleted(completeQuest))
        {
            if (const auto* storyQuest = storyQuests_.Find(completeQuest))
                wallet_->Earn(storyQuest->RewardMoney());
        }

        completedQuests_->Subscribe(completeQuest);
        storyQuests_.Remove(completeQuest);
    }

    bool QuestGroup::CheckCompleted(const QuestType& quest) const
    {
        return completedQuests_->CheckCompleted(quest);
    }
}
