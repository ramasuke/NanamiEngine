#include "Enemy_Behaviour_Action_IsCurrentHealthRate.h"
#include "../../../../../../Status/EnemyStatus.h"
#include <algorithm>

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::IsCurrentHealthRate::DoTick(const TickContext& context)
    {
        const auto& currentHealth = context.EnemyStatus().Health();
        const auto& maxHealth     = context.EnemyStatus().MaxHealth();

        float currentRate = (currentHealth.Value() / maxHealth) * 100.0f;
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
