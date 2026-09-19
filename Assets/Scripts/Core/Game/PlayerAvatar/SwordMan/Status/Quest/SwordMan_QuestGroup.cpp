#include "SwordMan_QuestGroup.h"

#include <algorithm>

#include "SwordMan_QuestContext.h"
#include "../../../Quest/PlayerAvatar_QuestContext.h"
#include "../../../Quest/PlayerAvatar_StoryQuestBase.h"
#include "../../../Quest/Completed/PlayerAvatar_CompletedQuestGroup.h"
#include "cereal/archives/binary.hpp"

namespace GameCore::PlayerAvatar::SwordMan
{
    QuestGroup::QuestGroup(
        const std::vector<std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>>& quests)
        : quests_(quests   )
        , completedQuests_(std::make_unique<Quest::CompletedQuestGroup>())
    {
    }

    QuestGroup::~QuestGroup() = default;

    void QuestGroup::Init(const std::shared_ptr<IObservableStatusEvent>& event,
                          const std::shared_ptr<IStatusEvent>& statusEvent,
                          const std::shared_ptr<IControlGuideFocusRequest>& guideFocus,
                          const std::shared_ptr<Wallet>& wallet)
    {
        event_       = event;
        statusEvent_ = statusEvent;
        guideFocus_  = guideFocus;
        wallet_      = wallet;

        for (const auto& quest : quests_)
        {
            quest->StartQuest(Npc::Friendly::Behaviour::Action::SwordManQuestContext{ *event_, *guideFocus_, *this });
        }
        storyQuests_.StartAll(Quest::QuestContext{ *statusEvent_, *this });
    }

    void QuestGroup::Subscribe(const std::shared_ptr<StoryQuestBase>& addQuest)
    {
        storyQuests_.Add(addQuest, Quest::QuestContext{ *statusEvent_, *this });
    }

    void QuestGroup::Subscribe(const std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>& addQuest)
    {
        quests_.push_back(addQuest);
        addQuest->StartQuest(Npc::Friendly::Behaviour::Action::SwordManQuestContext{ *event_, *guideFocus_, *this });
    }

    void QuestGroup::OnDrawGui()
    {
        for (const auto& quest : quests_)
        {
            quest->OnDrawGui();
        }
        storyQuests_.OnDrawGui();
    }

    std::unique_ptr<QuestGroup> QuestGroup::DeepCoy() const
    {
        std::stringstream ss;

        {
            cereal::BinaryOutputArchive outputArchive(ss);
            outputArchive(*this);
        }

        auto copy = std::make_unique<QuestGroup>();

        {
            cereal::BinaryInputArchive inputArchive(ss);
            inputArchive(*copy);
        }

        return copy;
    }

    bool QuestGroup::IsTaking(const QuestType& quest) const
    {
        const bool isSwordManQuest = std::ranges::any_of(quests_, [&quest](const auto& taking)
        {
            return taking->QuestType() == quest;
        });
        return isSwordManQuest || storyQuests_.Contains(quest);
    }

    void QuestGroup::CompleteQuest(const QuestType& completeQuest)
    {
        // 完了フラグはセーブをまたいで残るので、受け直しても報酬が出るのは初回だけ
        if (wallet_ && !completedQuests_->CheckCompleted(completeQuest))
        {
            const auto swordManQuest = std::ranges::find_if(quests_, [&completeQuest](const auto& quest)
            {
                return quest->QuestType() == completeQuest;
            });
            if (swordManQuest != quests_.end())
                wallet_->Earn((*swordManQuest)->RewardMoney());
            else if (const auto reward = storyQuests_.RewardOf(completeQuest))
                wallet_->Earn(*reward);
        }

        completedQuests_->Subscribe(completeQuest);
        std::erase_if(quests_, [completeQuest](const auto& quest)
        {
            return quest->QuestType() == completeQuest;
        });
        storyQuests_.Remove(completeQuest);
    }

    bool QuestGroup::CheckCompleted(const QuestType& quest) const
    {
        return completedQuests_->CheckCompleted(quest);
    }
}
