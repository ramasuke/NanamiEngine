#include "Enemy_Behaviour_Action_ShootDownAirShip.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ShootDownAirShip::DoTick(const TickContext& context)
    {
        shootDownAirShip_->OnShootDown();
        return TickStatus::Success;
    }

    void Action::ShootDownAirShip::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("shootDownAirShip_", shootDownAirShip_);
    }
}
