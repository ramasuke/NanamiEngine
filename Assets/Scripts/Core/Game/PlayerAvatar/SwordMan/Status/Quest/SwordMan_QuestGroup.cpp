#include "SwordMan_QuestGroup.h"

#include <algorithm>

#include "SwordMan_QuestContext.h"
#include "../../../Quest/PlayerAvatar_ITakeableQuest.h"
#include "../../../Quest/PlayerAvatar_QuestJournal.h"
#include "../../../Record/PlayerAvatar_RecordBook.h"
#include "cereal/archives/portable_binary.hpp"

namespace GameCore::PlayerAvatar::SwordMan
{
    QuestGroup::QuestGroup(
        const std::vector<std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>>& quests)
        : quests_(quests)
    {
    }

    QuestGroup::~QuestGroup() = default;

    void QuestGroup::Init(const std::shared_ptr<IObservableStatusEvent>& event,
                          const std::shared_ptr<IControlGuideFocusRequest>& guideFocus,
                          const std::shared_ptr<Wallet>& wallet)
    {
        event_       = event;
        guideFocus_  = guideFocus;
        wallet_      = wallet;

        for (const auto& quest : quests_)
        {
            quest->StartQuest(Npc::Friendly::Behaviour::Action::SwordManQuestContext{ *event_, *guideFocus_, *this });
        }
    }

    bool QuestGroup::Subscribe(const std::shared_ptr<Quest::ITakeableQuest>& addQuest)
    {
        return Quest::QuestJournal::Instance().Take(addQuest);
    }

    bool QuestGroup::Subscribe(const std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>& addQuest)
    {
        if (!addQuest || IsTaking(addQuest->QuestType()))
            return false;

        quests_.push_back(addQuest);
        addQuest->StartQuest(Npc::Friendly::Behaviour::Action::SwordManQuestContext{ *event_, *guideFocus_, *this });
        return true;
    }

    std::vector<std::shared_ptr<Quest::ITakeableQuest>> QuestGroup::ReleaseLegacyQuests()
    {
        return legacyStoryQuests_.Release();
    }

    void QuestGroup::OnDrawGui()
    {
        for (const auto& quest : quests_)
        {
            quest->OnDrawGui();
        }
        Quest::QuestJournal::Instance().OnDrawGui();
        Record::RecordBook::Instance().OnDrawGui();
    }

    std::unique_ptr<QuestGroup> QuestGroup::DeepCoy() const
    {
        std::stringstream ss;

        {
            // NOTE: 型登録が紐付くのは JSON と PortableBinary だけ
            cereal::PortableBinaryOutputArchive outputArchive(ss);
            outputArchive(*this);
        }

        auto copy = std::make_unique<QuestGroup>();

        {
            cereal::PortableBinaryInputArchive inputArchive(ss);
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
        return isSwordManQuest || Quest::QuestJournal::Instance().IsTaking(quest);
    }

    void QuestGroup::CompleteQuest(const QuestType& completeQuest)
    {
        const auto swordManQuest = std::ranges::find_if(quests_, [&completeQuest](const auto& quest)
        {
            return quest->QuestType() == completeQuest;
        });
        if (swordManQuest == quests_.end())
        {
            Quest::QuestJournal::Instance().CompleteQuest(completeQuest);
            return;
        }

        // 完了フラグはセーブをまたいで残るので、受け直しても報酬が出るのは初回だけ
        const auto reward = (*swordManQuest)->RewardMoney();
        if (Quest::QuestJournal::Instance().MarkCompleted(completeQuest) && wallet_)
            wallet_->Earn(reward);

        std::erase_if(quests_, [completeQuest](const auto& quest)
        {
            return quest->QuestType() == completeQuest;
        });
    }

    bool QuestGroup::CheckCompleted(const QuestType& quest) const
    {
        return Quest::QuestJournal::Instance().CheckCompleted(quest);
    }
}
