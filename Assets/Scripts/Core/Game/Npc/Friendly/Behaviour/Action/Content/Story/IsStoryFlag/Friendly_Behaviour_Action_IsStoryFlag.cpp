#include "Friendly_Behaviour_Action_IsStoryFlag.h"
#include "../../../../../../../Story/Story_StoryProgress.h"
#include "../../../../../../../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::IsStoryFlag::DoTick(const TickContext& context)
    {
        const auto flag = static_cast<Story::StoryFlag>(flag_);
        return Story::StoryProgress::Instance().IsSet(flag) == expected_ ? TickStatus::Success : TickStatus::Failure;
    }

    void Action::IsStoryFlag::DoDrawGui()
    {
        auto flag = static_cast<Story::StoryFlag>(flag_);
        ImGuiHelper::OnDrawEnumField("flag_", flag, Story::STORY_FLAGS, Story::ToString);
        flag_ = static_cast<int>(flag);
        ImGuiHelper::OnDrawInputField("expected_", expected_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::IsStoryFlag, GameCore::Npc::Friendly::Behaviour::ActionBase);
#pragma endregion
