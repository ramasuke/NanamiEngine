#include "SwordManAvatarInjuredRunState.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarInjuredRunState::DoEnter()
    {
        StatusEvent().InvokeOnRun();
        ResetMoveSpeedFromVelocity();
    }

    void SwordManAvatarInjuredRunState::DoFixedUpdate()
    {
        MoveForward(Status().GetRunSpeed(), Resources().RunAccelerationTime_secs(), Resources().RunDecelerationTime_secs());
    }

    void SwordManAvatarInjuredRunState::DoUpdate()
    {
        TryEmitFootstep(Resources().RunFootstepSounds());

        UpdateLockOn();
        UpdateItemPouchInput();
        UpdateTransitions();
    }

    void SwordManAvatarInjuredRunState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        visitor.Automatic(SwordManAvatarStateType::Hurt, Status().IsDamaged());
        visitor.Automatic(SwordManAvatarStateType::Run, !Status().IsInjured());
        visitor.OnInput(SwordManAvatarStateType::Idle, SwordManAvatarInput::Move, SwordManAvatarInputPhase::NotHolding, true);
        visitor.Automatic(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk,
                          !Input().Run().IsUpdatePressed() || !Status().CanRun());
        visitor.OnInput(SwordManAvatarStateType::Jump, SwordManAvatarInput::Jump, SwordManAvatarInputPhase::Pressed, Status().CanJump());
        visitor.OnInput(SwordManAvatarStateType::AvoidRolling, SwordManAvatarInput::AvoidRolling, SwordManAvatarInputPhase::Pressed, Status().CanAvoidRolling());
        visitor.Action(SwordManAvatarStateAction::Move, true);
        VisitLockOnAction(visitor);
        visitor.Action(SwordManAvatarStateAction::CycleItem, true);
        visitor.Action(SwordManAvatarStateAction::UseItem, Status().Pouch().CanUseSelected());
        visitor.OnInput(SwordManAvatarStateType::DashAttack, SwordManAvatarInput::DashAttack, SwordManAvatarInputPhase::Pressed, true);
        visitor.Automatic(SwordManAvatarStateType::UseCanon, Conditions().CanUseCannon());
        visitor.Automatic(SwordManAvatarStateType::Floating, !Conditions().IsGround());
    }

    void SwordManAvatarInjuredRunState::DoExit()
    {
    }
}
