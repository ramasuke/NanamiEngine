#include "PlayerAvatar_StoryUnlockConditions.h"

#include "PlayerAvatar_QuestUnlockConditionFactory.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../Story/Story_StoryProgress.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::Quest::Unlock
{
    bool StoryFlagUnlockCondition::IsSatisfied(const QuestUnlockContext& context) const
    {
        return context.story && context.story->IsSet(storyFlag_);
    }

    std::string StoryFlagUnlockCondition::Describe() const
    {
        return "StoryFlag " + std::string(Story::ToString(storyFlag_));
    }

    void StoryFlagUnlockCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("storyFlag_", storyFlag_, Story::STORY_FLAGS, Story::ToString);
    }

    bool FacilityRestoredUnlockCondition::IsSatisfied(const QuestUnlockContext& context) const
    {
        return context.story && context.story->IsRestored(facility_);
    }

    std::string FacilityRestoredUnlockCondition::Describe() const
    {
        return "FacilityRestored " + std::string(Story::ToString(facility_));
    }

    void FacilityRestoredUnlockCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField("facility_", facility_, Story::FACILITIES, Story::ToString);
    }

    REGISTER_QUEST_UNLOCK_CONDITION(StoryFlagUnlockCondition)
    REGISTER_QUEST_UNLOCK_CONDITION(FacilityRestoredUnlockCondition)
}

CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::Quest::Unlock::StoryFlagUnlockCondition)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::Quest::Unlock::IQuestUnlockCondition, GameCore::PlayerAvatar::Quest::Unlock::StoryFlagUnlockCondition)
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::Quest::Unlock::FacilityRestoredUnlockCondition)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::Quest::Unlock::IQuestUnlockCondition, GameCore::PlayerAvatar::Quest::Unlock::FacilityRestoredUnlockCondition)
