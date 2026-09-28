#include "Condition_QuestCompletedCondition.h"

#include "Condition_ConditionFactory.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../PlayerAvatar/Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Condition
{
    bool QuestCompletedCondition::IsSatisfied(const ConditionContext& context) const
    {
        return context.completedQuests && context.completedQuests->CheckCompleted(questType_);
    }

    std::string QuestCompletedCondition::Describe() const
    {
        return "QuestCompleted " + std::string(PlayerAvatar::ToString(questType_));
    }

    void QuestCompletedCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("questType_", questType_, PlayerAvatar::QUEST_TYPE_NAMES, PlayerAvatar::ToString);
    }

    REGISTER_CONDITION(QuestCompletedCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::QuestCompletedCondition, GameCore::Condition::ICondition);
