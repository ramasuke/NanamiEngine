#include "GamePlay_Enemy_Tyrannosaurus.h"

namespace GamePlay::Npc::Enemy
{
    void Tyrannosaurus::DoAwake()
    {
        if (!healthBar_)
            return;

        healthBar_->Entity().lock()->SetEnable(true);
        NetworkStatus()->Get().HealthObservable().subscribe(rxcpp::composite_subscription(), [&](const GameCore::StatusParameter::Health health)
        {
            healthBar_->SetValue(health / NetworkStatus()->Get().MaxHealth());
        });
    }

    void Tyrannosaurus::DoUpdate()
    {
    }

    void Tyrannosaurus::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("healthBar_", healthBar_);
    }
}
