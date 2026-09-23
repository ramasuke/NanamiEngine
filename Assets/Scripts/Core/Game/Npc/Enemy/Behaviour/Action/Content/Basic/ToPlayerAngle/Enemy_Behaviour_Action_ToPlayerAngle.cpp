#include "Enemy_Behaviour_Action_ToPlayerAngle.h"

#include <cmath>

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "../Common/Enemy_Behaviour_FaceDirection.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ToPlayerAngle::DoTick(const TickContext& context)
    {
        const auto& transform = context.EnemyTransform();
        glm::vec3 playerPos;
        if (!context.NearestPlayerPosition(transform.GetWorldPos(), playerPos))
            return TickStatus::Failure;

        float angle;
        if (!SignedHorizontalAngleDeg(transform, playerPos - transform.GetWorldPos(), angle))
        return TickStatus::Failure;

        if (useAbsolute_)
            angle = std::abs(angle);
        return minDegree_ <= angle && angle <= maxDegree_ ? TickStatus::Success : TickStatus::Failure;
    }

    void Action::ToPlayerAngle::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("minDegree_", minDegree_);
        ImGuiHelper::OnDrawInputField("maxDegree_", maxDegree_);
        ImGuiHelper::OnDrawInputField("useAbsolute_", useAbsolute_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ToPlayerAngle);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ToPlayerAngle);
#pragma endregion
