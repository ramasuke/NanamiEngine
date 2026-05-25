#include "Enemy_Behaviour_Action_CheckCompleteQuest.h"

#include "../../../../../../../PlayerAvatar/Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../../../../../PlayerAvatar/Quest/Completed/PlayerAvatar_CompletedQuestGroup.h"
#include "../../../../../../../PlayerAvatar/Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::CheckCompleteQuest::DoTick(
        const TickContext& context)
    {
        const auto isCompleted = context.PlayerCompleteQuest().CheckCompleted(questType_);

        return isCompleted ? TickStatus::Success : TickStatus::Failure;
    }

    void Action::CheckCompleteQuest::DoDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField(
            "Quest Type",
            questType_,
            PlayerAvatar::QUEST_TYPE_NAMES,
            PlayerAvatar::ToString
        );
    }    
}
