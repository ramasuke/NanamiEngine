#include "Condition_FacilityRestoredCondition.h"

#include "Condition_ConditionFactory.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../Story/Story_StoryProgress.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Condition
{
    bool FacilityRestoredCondition::IsSatisfied(const ConditionContext& context) const
    {
        return context.story.IsRestored(facility_);
    }

    std::string FacilityRestoredCondition::Describe() const
    {
        return "FacilityRestored " + std::string(Story::ToString(facility_));
    }

    void FacilityRestoredCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("facility_", facility_, Story::FACILITIES, Story::ToString);
    }

    REGISTER_CONDITION(FacilityRestoredCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::FacilityRestoredCondition, GameCore::Condition::ICondition);
