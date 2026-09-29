#include "Condition_StoryFlagCondition.h"

#include "Condition_ConditionFactory.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../Story/Story_StoryProgress.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Condition
{
    bool StoryFlagCondition::IsSatisfied(const ConditionContext& context) const
    {
        return context.story.IsSet(storyFlag_);
    }

    std::string StoryFlagCondition::Describe() const
    {
        return "StoryFlag " + std::string(Story::ToString(storyFlag_));
    }

    void StoryFlagCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("storyFlag_", storyFlag_, Story::STORY_FLAGS, Story::ToString);
    }

    REGISTER_CONDITION(StoryFlagCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::StoryFlagCondition, GameCore::Condition::ICondition);
