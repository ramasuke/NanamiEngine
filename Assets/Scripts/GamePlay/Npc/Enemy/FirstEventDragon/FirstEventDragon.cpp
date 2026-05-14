#include "FirstEventDragon.h"

#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Npc::Enemy
{
    void FirstEventDragon::DoAwake()
    {
        healthBar_->Entity().lock()->SetEnable(true);
        Status().Health().Subscribe(rxcpp::composite_subscription(), [&](const GameCore::StatusParameter::Health health)
        {
            healthBar_->SetValue(health / Status().MaxHealth());
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
        ImGuiHelper::OnDrawInputField("healthBar_", healthBar_);
    }
}