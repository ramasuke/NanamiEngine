#include "Enemy_Behaviour_Action_ToPlayerDistance.h"

#include "../../../../../../../PlayerAvatar/IPlayerAvatar.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ToPlayerDistance::DoTick(const TickContext& context)
    {
        const glm::vec3 selfPos   = context.EnemyTransform().GetWorldPos();
        const glm::vec3 playerPos = context.Player()->PlayerTransform().GetWorldPos();

        const glm::vec3 diff   = playerPos - selfPos;
        const float     distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        bool isInner = distSq <= distance_ * distance_;
        return isInner == isInnerDistance_ ? TickStatus::Success : TickStatus::Failure; 
    }

    void Action::ToPlayerDistance::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("distance_", distance_);
        ImGuiHelper::OnDrawInputField("isInnerDistance_", isInnerDistance_);
    }
}
