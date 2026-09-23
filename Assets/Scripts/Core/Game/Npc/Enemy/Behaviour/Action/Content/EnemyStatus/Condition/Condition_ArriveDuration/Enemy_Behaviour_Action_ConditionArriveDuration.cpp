#include "Enemy_Behaviour_Action_ConditionArriveDuration.h"

#include "../../../../../../Status/EnemyStatus.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ConditionArriveDuration::DoTick(
        const TickContext& context)
    {
        if (context.EnemyStatus()->Get().ArriveDuration_secs() >= arriveDuration_secs_)
        {
             return TickStatus::Success;
        }
        return TickStatus::Failure;
    }
    
    void Action::ConditionArriveDuration::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("arriveDuration_secs_", arriveDuration_secs_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ConditionArriveDuration)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ConditionArriveDuration)
#pragma endregion
