#include "SwordManAvatar_WarpInState.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void WarpInState::DoEnter()
    {
        // 演出が書いたTransformを物理に押し戻されないようKinematicにする。DoExitでDynamicへ戻す
        RigidBody().SetMotionType(Physics::MotionType::Kinematic);
        RigidBody().SetGravity(false);
        RigidBody().SetLinearVelocity(glm::vec3(0.0f));
        footstep_ = {};
    }

    void WarpInState::DoFixedUpdate()
    {
        RigidBody().SetLinearVelocity(glm::vec3(0.0f));
    }

    void WarpInState::DoUpdate()
    {
        // 出現中に食らったダメージは持ち越さない
        Status().DiscardDamage();

        TryEmitFootstep(footstep_, Resources().WalkFootstepSounds());
    }

    void WarpInState::DoExit()
    {
        RigidBody().SetGravity(true);
        RigidBody().SetMotionType(Physics::MotionType::Dynamic);
        RigidBody().SetLinearVelocity(glm::vec3(0.0f));
    }
}
