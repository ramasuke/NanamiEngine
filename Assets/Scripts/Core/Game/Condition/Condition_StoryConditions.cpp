#include "Condition_StoryConditions.h"

#include "Condition_ConditionFactory.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../Story/Story_StoryProgress.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Condition
{
    bool StoryFlagCondition::IsSatisfied(const ConditionContext& context) const
    {
        return context.story && context.story->IsSet(storyFlag_);
    }

    std::string StoryFlagCondition::Describe() const
    {
        return "StoryFlag " + std::string(Story::ToString(storyFlag_));
    }

    void StoryFlagCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("storyFlag_", storyFlag_, Story::STORY_FLAGS, Story::ToString);
    }

    bool FacilityRestoredCondition::IsSatisfied(const ConditionContext& context) const
    {
        return context.story && context.story->IsRestored(facility_);
    }

    std::string FacilityRestoredCondition::Describe() const
    {
        return "FacilityRestored " + std::string(Story::ToString(facility_));
    }

    void FacilityRestoredCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("facility_", facility_, Story::FACILITIES, Story::ToString);
    }

    REGISTER_CONDITION(StoryFlagCondition)
    REGISTER_CONDITION(FacilityRestoredCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::StoryFlagCondition, GameCore::Condition::ICondition);
NANAMI_REGISTER_TYPE(GameCore::Condition::FacilityRestoredCondition, GameCore::Condition::ICondition);
