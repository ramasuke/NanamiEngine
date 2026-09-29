#include "Condition_NotCondition.h"

#include "Condition_ConditionFactory.h"
#include "Condition_ConditionList.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Condition
{
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

    REGISTER_CONDITION(NotCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::NotCondition, GameCore::Condition::ICondition);
