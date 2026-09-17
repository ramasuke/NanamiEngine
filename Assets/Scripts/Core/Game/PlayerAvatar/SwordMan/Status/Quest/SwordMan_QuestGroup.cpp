#include "SwordMan_QuestGroup.h"

#include "SwordMan_QuestContext.h"
#include "../../../Quest/PlayerAvatar_QuestBase.h"
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
                          const std::shared_ptr<IControlGuideFocusRequest>& guideFocus,
                          const std::shared_ptr<Wallet>& wallet)
    {
        event_      = event;
        guideFocus_ = guideFocus;
        wallet_     = wallet;

        for (const auto& quest : quests_)
        {
            quest->StartQuest(Npc::Friendly::Behaviour::Action::SwordManQuestContext{ *event_, *guideFocus_, *this });
        }
    }

    void QuestGroup::Subscribe(const std::shared_ptr<QuestBase>& addQuest)
    {
        Subscribe(std::static_pointer_cast<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>(addQuest));
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

    void QuestGroup::CompleteQuest(const QuestType& completeQuest)
    {
        // 完了フラグはセーブをまたいで残るので、受け直しても報酬が出るのは初回だけ
        if (wallet_ && !completedQuests_->CheckCompleted(completeQuest))
        {
            for (const auto& quest : quests_)
            {
                if (quest->QuestType() != completeQuest)
                    continue;

                wallet_->Earn(quest->RewardMoney());
                break;
            }
        }

        completedQuests_->Subscribe(completeQuest);
        std::erase_if(quests_, [completeQuest](const auto& quest)
        {
            return quest->QuestType() == completeQuest;
        });
    }

    bool QuestGroup::CheckCompleted(const QuestType& quest) const
    {
        return completedQuests_->CheckCompleted(quest);
    }
}
