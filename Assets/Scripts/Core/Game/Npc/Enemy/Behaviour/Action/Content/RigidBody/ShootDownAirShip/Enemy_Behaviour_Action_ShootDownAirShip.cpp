#include "Enemy_Behaviour_Action_ShootDownAirShip.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ShootDownAirShip, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
