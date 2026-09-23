#include "Enemy_Behaviour_Action_IsCurrentHealthRate.h"
#include "../../../../../../Status/EnemyStatus.h"
#include <algorithm>
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::IsCurrentHealthRate::DoTick(const TickContext& context)
    {
        const auto& currentHealth = context.EnemyStatus()->Get().Health();
        const auto& maxHealth     = context.EnemyStatus()->Get().MaxHealth();

        float currentRate = currentHealth / maxHealth * 100.0f;
        currentRate = std::clamp(currentRate, 0.0f, 100.0f);

        if (currentRate <= rate_)
        {
            return TickStatus::Success;
        }

        return TickStatus::Failure;
    }

    void Action::IsCurrentHealthRate::DoDrawGui()
    {
        ImGui::SliderFloat("Health Threshold (%)", &rate_, 0.0f, 100.0f, "%.1f %%");
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::IsCurrentHealthRate)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::IsCurrentHealthRate)
#pragma endregion
