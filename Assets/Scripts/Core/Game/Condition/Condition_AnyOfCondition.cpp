#include "Condition_AnyOfCondition.h"

#include <algorithm>

#include "Condition_ConditionFactory.h"
#include "Condition_ConditionList.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Condition
{
    bool AnyOfCondition::IsSatisfied(const ConditionContext& context) const
    {
        return std::ranges::any_of(conditions_, [&context](const auto& condition)
        {
            return condition && condition->IsSatisfied(context);
        });
    }

    std::string AnyOfCondition::Describe() const
    {
        return "AnyOf (" + std::to_string(conditions_.size()) + ")";
    }

    void AnyOfCondition::OnDrawGui()
    {
        ConditionList::DrawListGui("conditions_", conditions_);
    }

    REGISTER_CONDITION(AnyOfCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::AnyOfCondition, GameCore::Condition::ICondition);
