#include "SwordManAvatarInjuredWalkState.h"

#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarInjuredWalkState::DoEnter()
    {
        StatusEvent().InvokeOnMove();
        ResetMoveSpeedFromVelocity();
    }

    void SwordManAvatarInjuredWalkState::DoFixedUpdate()
    {
        MoveForward(
            Status().GetWalkSpeed(),
            Resources().WalkAccelerationTime_secs(),
            Resources().WalkDecelerationTime_secs());
    }

    void SwordManAvatarInjuredWalkState::DoUpdate()
    {
        TryEmitFootstep(Resources().WalkFootstepSounds());

        UpdateLockOn();
        UpdateItemPouchInput();
        UpdateTransitions();
    }

    void SwordManAvatarInjuredWalkState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        visitor.Automatic(SwordManAvatarStateType::Hurt, Status().IsDamaged());
        visitor.Automatic(SwordManAvatarStateType::Walk, !Status().IsInjured());
        visitor.OnInput(SwordManAvatarStateType::Idle, SwordManAvatarInput::Move, SwordManAvatarInputPhase::NotHolding, true);
        visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredRun : SwordManAvatarStateType::Run,
                        SwordManAvatarInput::Run, SwordManAvatarInputPhase::Holding, Status().CanRun());
        visitor.OnInput(SwordManAvatarStateType::Jump, SwordManAvatarInput::Jump, SwordManAvatarInputPhase::Pressed, Status().CanJump());
        visitor.OnInput(SwordManAvatarStateType::AvoidRolling, SwordManAvatarInput::AvoidRolling, SwordManAvatarInputPhase::Pressed, Status().CanAvoidRolling());
        visitor.Action(SwordManAvatarStateAction::Move, true);
        VisitLockOnAction(visitor);
        visitor.Action(SwordManAvatarStateAction::CycleItem, true);
        visitor.Action(SwordManAvatarStateAction::UseItem, Status().Pouch().CanUseSelected());
        visitor.OnInput(SwordManAvatarStateType::NormalAttack, SwordManAvatarInput::NormalAttack, SwordManAvatarInputPhase::Pressed, true);
        visitor.Automatic(SwordManAvatarStateType::UseCanon, Conditions().CanUseCannon());
        visitor.Automatic(SwordManAvatarStateType::Floating, !Conditions().IsGround());
    }

    void SwordManAvatarInjuredWalkState::DoExit()
    {
    }
}
