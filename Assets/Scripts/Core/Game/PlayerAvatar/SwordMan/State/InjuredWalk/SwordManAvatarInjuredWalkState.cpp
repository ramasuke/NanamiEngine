#include "SwordManAvatarInjuredWalkState.h"

#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarInjuredWalkState::DoEnter()
    {
        ResetMoveSpeedFromVelocity();
    }

    void SwordManAvatarInjuredWalkState::DoFixedUpdate()
    {
        AcceleratedForwardMove(Status().GetWalkSpeed(), Status().WalkAccelerationTime_secs());
    }

    void SwordManAvatarInjuredWalkState::DoUpdate()
    {
        TryEmitFootstep(Resources().WalkFootstepContactPhases(), Resources().WalkFootstepSounds());

        if (Status().IsDamaged())
            OnChangeState(SwordManAvatarStateType::Hurt);
        if (!Status().IsInjured())
            OnChangeState(SwordManAvatarStateType::Walk);
        if (!Input().Move().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Idle);
        if (Input().Run().IsUpdatePressed() && Status().CanRun())
            OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredRun : SwordManAvatarStateType::Run);
        if (Input().Jump().IsPressed() && Status().CanJump())
            OnChangeState(SwordManAvatarStateType::Jump);
        if (Input().AvoidRolling().IsPressed() && Status().CanAvoidRolling())
            OnChangeState(SwordManAvatarStateType::AvoidRolling);
        UpdateLockOn();
        if (Input().NormalAttack().IsPressed())
            OnChangeState(SwordManAvatarStateType::NormalAttack);
        if (Conditions().CanUseCannon())
            OnChangeState(SwordManAvatarStateType::UseCanon);
        if (!Conditions().IsGround())
            OnChangeState(SwordManAvatarStateType::Floating);
    }

    void SwordManAvatarInjuredWalkState::DoExit()
    {
    }
}
