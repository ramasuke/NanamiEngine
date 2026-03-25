#include "Prop_AirShip.h"

#include "../../../../../Engine/Module/Component/Collider/ICollider.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Body/MotionType.h"

namespace GamePlay::Prop
{
    void AirShip::OnShootDown()
    {
        for (const auto& collider : Components().Catches<Physics::ICollider>())
        {
            collider.lock()->ChangeEmotionType(JPH::EMotionType::Dynamic);
        }
        
        shootDownParticle_->SetEnable(true);
    }

    void AirShip::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("shootDownParticle_", shootDownParticle_);
    }
}
