#include "GamePlay_Enemy_FirstEventDragon.h"

#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Npc::Enemy
{
    void FirstEventDragon::DoAwake()
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

    void FirstEventDragon::DoUpdate()
    {
        if (Transform().GetWorldPos().y < -100)
        {
            Transform().SetLocalPos(glm::vec3{0.0f, 300.0f, 0.0f});
        }
        
    }

    void FirstEventDragon::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bossHealthGauge_", bossHealthGauge_);
        ImGuiHelper::OnDrawInputField("bossName_", bossName_);
    }
}