#include "PlayerAvatar_CompositeUnlockConditions.h"

#include <algorithm>

#include "PlayerAvatar_QuestUnlockConditionFactory.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::Quest::Unlock
{
    bool AnyOfUnlockCondition::IsSatisfied(const QuestUnlockContext& context) const
    {
        return std::ranges::any_of(conditions_, [&context](const auto& condition)
        {
            return condition && condition->IsSatisfied(context);
        });
    }

    std::string AnyOfUnlockCondition::Describe() const
    {
        return "AnyOf (" + std::to_string(conditions_.size()) + ")";
    }

    void AnyOfUnlockCondition::OnDrawGui()
    {
        QuestUnlockConditionList::DrawListGui("conditions_", conditions_);
    }

    bool NotUnlockCondition::IsSatisfied(const QuestUnlockContext& context) const
    {
        return !condition_ || !condition_->IsSatisfied(context);
    }

    std::string NotUnlockCondition::Describe() const
    {
        return condition_ ? "Not " + condition_->Describe() : "Not";
    }

    void NotUnlockCondition::OnDrawGui()
    {
        QuestUnlockConditionList::DrawSingleGui("condition_", condition_);
    }

    REGISTER_QUEST_UNLOCK_CONDITION(AnyOfUnlockCondition)
    REGISTER_QUEST_UNLOCK_CONDITION(NotUnlockCondition)
}

NANAMI_REGISTER_TYPE(GameCore::PlayerAvatar::Quest::Unlock::AnyOfUnlockCondition, GameCore::PlayerAvatar::Quest::Unlock::IQuestUnlockCondition);
NANAMI_REGISTER_TYPE(GameCore::PlayerAvatar::Quest::Unlock::NotUnlockCondition, GameCore::PlayerAvatar::Quest::Unlock::IQuestUnlockCondition);
