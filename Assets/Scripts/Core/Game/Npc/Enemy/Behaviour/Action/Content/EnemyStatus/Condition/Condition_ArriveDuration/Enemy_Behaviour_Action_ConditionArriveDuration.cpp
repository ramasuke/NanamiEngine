#include "Enemy_Behaviour_Action_ConditionArriveDuration.h"

#include "../../../../../../Status/EnemyStatus.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ConditionArriveDuration::DoTick(
        const TickContext& context)
    {
        if (context.EnemyStatus().ArriveDuration_secs() >= arriveDuration_secs_)
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
