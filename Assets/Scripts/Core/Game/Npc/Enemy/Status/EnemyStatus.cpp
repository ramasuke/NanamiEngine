#include "EnemyStatus.h"

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Npc::Enemy
{
    EnemyStatus::EnemyStatus(const int healthValue)
        : health_(StatusParameter::Health(healthValue))
    {
    }

    void EnemyStatus::ManualUpdate()
    {
        arriveDuring_secs_ += NanamiEngine::Time::DeltaTime();
    }

    void EnemyStatus::OnDamage(const int damageValue)
    {
        health_.OnNext(StatusParameter::Health(health_.get().Value() - damageValue));
    }

    void EnemyStatus::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("maxHealth_", maxHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("health_"   , health_   );
    }
}
