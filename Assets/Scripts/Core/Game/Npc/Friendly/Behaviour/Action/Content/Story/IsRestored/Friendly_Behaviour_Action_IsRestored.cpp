#include "Friendly_Behaviour_Action_IsRestored.h"
#include "../../../../../../../Story/Story_StoryProgress.h"
#include "../../../../../../../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::IsRestored::DoTick(const TickContext& context)
    {
        const auto facility = static_cast<Story::Facility>(facility_);
        return Story::StoryProgress::Instance().IsRestored(facility) == expected_ ? TickStatus::Success : TickStatus::Failure;
    }

    void Action::IsRestored::DoDrawGui()
    {
        auto facility = static_cast<Story::Facility>(facility_);
        ImGuiHelper::OnDrawEnumField("facility_", facility, Story::FACILITIES, Story::ToString);
        facility_ = static_cast<int>(facility);
        ImGuiHelper::OnDrawInputField("expected_", expected_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::IsRestored);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::IsRestored);
#pragma endregion
