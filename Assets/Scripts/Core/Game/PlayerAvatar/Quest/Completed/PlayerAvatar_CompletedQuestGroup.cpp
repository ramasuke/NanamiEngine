#include "PlayerAvatar_CompletedQuestGroup.h"

#include <cereal/types/unordered_set.hpp>
#include "../../../../../../../Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace GameCore::PlayerAvatar::Quest
{
    CompletedQuestGroup::CompletedQuestGroup()
    {
        completedQuests_ = LocalPrefs::LoadOrDefault<std::unordered_set<QuestType>>(
            COMPLETED_QUEST_SAVE_KEY,
            std::unordered_set<QuestType>());
    }

    void CompletedQuestGroup::Subscribe(const QuestType& completeQuest)
    {
        completedQuests_.insert(completeQuest);
        LocalPrefs::Save(COMPLETED_QUEST_SAVE_KEY, completedQuests_);
    }

    bool CompletedQuestGroup::CheckCompleted(const QuestType& quest) const
    {
        return completedQuests_.contains(quest);
    }
}
