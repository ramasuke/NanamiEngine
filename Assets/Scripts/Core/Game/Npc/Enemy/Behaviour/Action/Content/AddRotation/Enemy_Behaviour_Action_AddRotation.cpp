#include "Enemy_Behaviour_Action_AddRotation.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::AddRotation::DoTick(const TickContext& context)
    {
        auto& transform = context.EnemyTransform();

        const glm::quat currentRot = transform.GetWorldRot();

        const float yawRad = glm::radians(yawStep_degree_);
        const glm::quat addRot = glm::angleAxis(yawRad, glm::vec3(0, 1, 0));
        const glm::quat nextRot = addRot * currentRot;

        transform.SetWorldRot(nextRot);

        return TickStatus::Running;
    }

    void Action::AddRotation::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField(
            "Yaw Step (deg / tick)",
            yawStep_degree_
        );
    }
}
