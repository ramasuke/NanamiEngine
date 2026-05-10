#include "Prop_AirShip.h"

#include "../../../../../Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "Jolt/Physics/Body/MotionType.h"

namespace GamePlay::Prop
{
    void AirShip::OnShootDown()
    {
        for (const auto& collider : Components().Catches<Component::ColliderBase>())
        {
            collider.lock()->SetMotionType(JPH::EMotionType::Dynamic);
        }
        
        shootDownParticle_->SetEnable(true);
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
