#include "PlayerAvatar_QuestCompletedUnlockCondition.h"

#include "PlayerAvatar_QuestUnlockConditionFactory.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::Quest::Unlock
{
    bool QuestCompletedUnlockCondition::IsSatisfied(const QuestUnlockContext& context) const
    {
        return context.completedQuests && context.completedQuests->CheckCompleted(questType_);
    }

    std::string QuestCompletedUnlockCondition::Describe() const
    {
        return "QuestCompleted " + std::string(PlayerAvatar::ToString(questType_));
    }

    void QuestCompletedUnlockCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("questType_", questType_, PlayerAvatar::QUEST_TYPE_NAMES, PlayerAvatar::ToString);
    }

    REGISTER_QUEST_UNLOCK_CONDITION(QuestCompletedUnlockCondition)
}

NANAMI_REGISTER_TYPE(GameCore::PlayerAvatar::Quest::Unlock::QuestCompletedUnlockCondition, GameCore::PlayerAvatar::Quest::Unlock::IQuestUnlockCondition);
