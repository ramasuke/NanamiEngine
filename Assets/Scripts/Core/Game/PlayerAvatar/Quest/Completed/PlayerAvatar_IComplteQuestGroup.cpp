#include "PlayerAvatar_IComplteQuestGroup.h"

#include "../PlayerAvatar_ITakeableQuest.h"

namespace GameCore::PlayerAvatar::Quest
{
    void ICompleteQuestGroup::CompleteTakenQuest(const ITakeableQuest& quest)
    {
        CompleteQuest(quest.QuestType());
    }
}
