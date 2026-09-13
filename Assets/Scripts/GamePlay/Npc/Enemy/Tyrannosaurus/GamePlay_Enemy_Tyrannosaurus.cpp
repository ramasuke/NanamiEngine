#include "GamePlay_Enemy_Tyrannosaurus.h"

namespace GamePlay::Npc::Enemy
{
    void Tyrannosaurus::DoAwake()
    {
        if (!bossHealthGauge_)
            return;

        bossHealthGauge_->Show(bossName_);
        bossHealthGauge_->SetHealthRate(NetworkStatus()->Get().Health() / NetworkStatus()->Get().MaxHealth());
        NetworkStatus()->Get().HealthObservable().subscribe(rxcpp::composite_subscription(), [&](const GameCore::StatusParameter::Health health)
        {
            bossHealthGauge_->SetHealthRate(health / NetworkStatus()->Get().MaxHealth());
        });
    }

    void Tyrannosaurus::DoUpdate()
    {
    }

    void Tyrannosaurus::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bossHealthGauge_", bossHealthGauge_);
        ImGuiHelper::OnDrawInputField("bossName_", bossName_);
    }
}
