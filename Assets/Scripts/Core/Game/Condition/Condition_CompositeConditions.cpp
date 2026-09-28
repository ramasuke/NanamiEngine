#include "Condition_CompositeConditions.h"

#include <algorithm>

#include "Condition_ConditionFactory.h"
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

    bool NotCondition::IsSatisfied(const ConditionContext& context) const
    {
        return !condition_ || !condition_->IsSatisfied(context);
    }

    std::string NotCondition::Describe() const
    {
        return condition_ ? "Not " + condition_->Describe() : "Not";
    }

    void NotCondition::OnDrawGui()
    {
        ConditionList::DrawSingleGui("condition_", condition_);
    }

    REGISTER_CONDITION(AnyOfCondition)
    REGISTER_CONDITION(NotCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::AnyOfCondition, GameCore::Condition::ICondition);
NANAMI_REGISTER_TYPE(GameCore::Condition::NotCondition, GameCore::Condition::ICondition);
