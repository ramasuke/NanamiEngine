#include "PlayerAvatar_CompletedQuestGroup.h"

#include <cereal/types/unordered_set.hpp>
#include "../../../../../../../Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"

namespace GameCore::PlayerAvatar::Quest
{
    CompletedQuestGroup::CompletedQuestGroup()
    {
        try
        {
            completedQuests_ = NanamiEngine::Module::LocalPrefs::Load<std::unordered_set<QuestType>>(COMPLETED_QUEST_SAVE_KEY);
        }
        catch (...)
        {
            completedQuests_.clear();
        }
    }

    void CompletedQuestGroup::Subscribe(const QuestType& completeQuest)
    {
        completedQuests_.insert(completeQuest);
        NanamiEngine::Module::LocalPrefs::Save(COMPLETED_QUEST_SAVE_KEY, completedQuests_);
    }

    bool CompletedQuestGroup::CheckCompleted(const QuestType& quest) const
    {
        return completedQuests_.contains(quest);
    }
}
