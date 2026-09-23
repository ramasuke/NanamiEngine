#include "Friendly_Behaviour_Action_SetStoryFlag.h"
#include "../../../../../../../Story/Story_StoryProgress.h"
#include "../../../../../../../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::SetStoryFlag::DoTick(const TickContext& context)
    {
        Story::StoryProgress::Instance().Set(static_cast<Story::StoryFlag>(flag_));
        return TickStatus::Success;
    }

    void Action::SetStoryFlag::DoDrawGui()
    {
        auto flag = static_cast<Story::StoryFlag>(flag_);
        ImGuiHelper::OnDrawEnumField("flag_", flag, Story::STORY_FLAGS, Story::ToString);
        flag_ = static_cast<int>(flag);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::SetStoryFlag);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::SetStoryFlag);
#pragma endregion
