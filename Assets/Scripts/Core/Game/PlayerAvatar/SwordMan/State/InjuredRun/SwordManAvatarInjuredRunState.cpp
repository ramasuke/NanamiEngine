#include "SwordManAvatarInjuredRunState.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarInjuredRunState::DoEnter()
    {
        StatusEvent().InvokeOnRun();
        Status().SetIsRunning(true);
        ResetMoveSpeedFromVelocity();
    }

    void SwordManAvatarInjuredRunState::DoFixedUpdate()
    {
        AcceleratedForwardMove(Status().GetRunSpeed(), Status().RunAccelerationTime_secs());
    }

    void SwordManAvatarInjuredRunState::DoUpdate()
    {
        TryEmitFootstep(Resources().RunFootstepContactPhases(), Resources().RunFootstepSounds());

        if (Status().IsDamaged())
            OnChangeState(SwordManAvatarStateType::Hurt);
        if (!Status().IsInjured())
            OnChangeState(SwordManAvatarStateType::Run);
        if (!Input().Move().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Idle);
        if (!Input().Run().IsUpdatePressed() || !Status().CanRun())
            OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk);
        if (Input().Jump().IsPressed() && Status().CanJump())
            OnChangeState(SwordManAvatarStateType::Jump);
        if (Input().AvoidRolling().IsPressed() && Status().CanAvoidRolling())
            OnChangeState(SwordManAvatarStateType::AvoidRolling);
        UpdateLockOn();
        if (Input().DashAttack().IsPressed())
            OnChangeState(SwordManAvatarStateType::DashAttack);
        if (Conditions().CanUseCannon())
            OnChangeState(SwordManAvatarStateType::UseCanon);
        if (!Conditions().IsGround())
            OnChangeState(SwordManAvatarStateType::Floating);
    }

    void SwordManAvatarInjuredRunState::DoExit()
    {
        Status().SetIsRunning(false);
    }
}
