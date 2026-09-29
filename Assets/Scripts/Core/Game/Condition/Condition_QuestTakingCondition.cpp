#include "Condition_QuestTakingCondition.h"

#include "Condition_ConditionFactory.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../PlayerAvatar/Quest/PlayerAvatar_QuestJournal.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Condition
{
    bool QuestTakingCondition::IsSatisfied(const ConditionContext& context) const
    {
        // NOTE: 受注は手元のプレイヤーの1冊しか無いので、コンテキストを通さず直接見る
        return PlayerAvatar::Quest::QuestJournal::Instance().IsTaking(questType_);
    }

    std::string QuestTakingCondition::Describe() const
    {
        return "QuestTaking " + std::string(PlayerAvatar::ToString(questType_));
    }

    void QuestTakingCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("questType_", questType_, PlayerAvatar::QUEST_TYPE_NAMES, PlayerAvatar::ToString);
    }

    REGISTER_CONDITION(QuestTakingCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::QuestTakingCondition, GameCore::Condition::ICondition);
