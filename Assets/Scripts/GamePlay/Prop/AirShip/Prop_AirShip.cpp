#include "Prop_AirShip.h"

#include "Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    void AirShip::OnShootDown()
    {
        
    }

    void AirShip::OnAwake()
    {
        originPos_ = Transform().GetWorldPos();
    }

    void AirShip::OnUpdate()
    {
        
    }

    void AirShip::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("originPos_", originPos_);
        ImGuiHelper::OnDrawInputField("shootDownParticle_", shootDownParticle_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::AirShip);
#pragma endregion
