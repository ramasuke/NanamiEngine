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
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ShootDownAirShip)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ShootDownAirShip)
#pragma endregion
