#include "SwordMan_QuestGroup.h"

#include "../../../Quest/PlayerAvatar_QuestBase.h"
#include "../../../Quest/Completed/PlayerAvatar_CompletedQuestGroup.h"
#include "cereal/archives/binary.hpp"

namespace GameCore::PlayerAvatar::SwordMan
{
    QuestGroup::QuestGroup(
        const std::vector<std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>>& quests)
        : quests_(quests   )
    {
    }

    QuestGroup::~QuestGroup() = default;

    void QuestGroup::Init(const std::shared_ptr<IObservableStatusEvent>& event)
    {
        event_ = event;
        
        for (const auto& quest : quests_)
        {
            quest->StartQuest(*event_, *completedQuests_);
        }
    }

    void QuestGroup::Subscribe(const std::shared_ptr<QuestBase>& addQuest)
    {
        Subscribe(std::static_pointer_cast<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>(addQuest));
    }

    void QuestGroup::Subscribe(const std::shared_ptr<Npc::Friendly::Behaviour::Action::ITakeableSwordManQuest>& addQuest)
    {
        quests_.push_back(addQuest);
        addQuest->StartQuest(*event_, *completedQuests_);
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
}
