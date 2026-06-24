#include "EnemyStatus.h"

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Npc::Enemy
{
    void EnemyStatus::ManualUpdate()
    {
        arriveDuring_secs_ += NanamiEngine::Time::DeltaTime();
    }

    void EnemyStatus::OnDamage(const int damageValue)
    {
        currentHealth_->Set(StatusParameter::Health(currentHealth_->Get().Value() - damageValue));
    }

    void EnemyStatus::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("maxHealth_", maxHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("currentHealth_", currentHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("arriveDuring_secs_", arriveDuring_secs_);
    }
}
