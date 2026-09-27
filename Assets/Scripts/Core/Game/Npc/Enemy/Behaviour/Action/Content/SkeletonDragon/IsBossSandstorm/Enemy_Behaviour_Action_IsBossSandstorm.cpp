#include "Enemy_Behaviour_Action_IsBossSandstorm.h"
#include "../../../../../../../../../GamePlay/Weather/Sandstorm.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::IsBossSandstorm::DoTick(const TickContext& context)
    {
        return GamePlay::Weather::Sandstorm::IsSummoned() == isActive_ ? TickStatus::Success : TickStatus::Failure;
    }

    void Action::IsBossSandstorm::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("isActive_", isActive_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::IsBossSandstorm, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
