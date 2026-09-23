#include "Friendly_Behaviour_Action_CheckCompleteQuest.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../../../../../../PlayerAvatar/Quest/PlayerAvatar_IQuestGroup.h"
#include "../../../../../../../../PlayerAvatar/Quest/Completed/PlayerAvatar_CompletedQuestGroup.h"
#include "../../../../../../../../PlayerAvatar/Quest/Completed/PlayerAvatar_IComplteQuestGroup.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Friendly::Behaviour
{
    TickStatus Action::CheckCompleteQuest::DoTick(
        const TickContext& context)
    {
        const auto isCompleted = context.PlayerCompleteQuest().CheckCompleted(questType_);

        return isCompleted ? TickStatus::Success : TickStatus::Failure;
    }

    void Action::CheckCompleteQuest::DoDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawEnumField(
            "Quest Type",
            questType_,
            PlayerAvatar::QUEST_TYPE_NAMES,
            PlayerAvatar::ToString
        );
    }    
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::CheckCompleteQuest)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::CheckCompleteQuest)
#pragma endregion
