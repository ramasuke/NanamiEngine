#include "SwordManAvatarWalkState.h"

#include "Engine/Module/Component/Animator/Animator.h"
#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarWalkState::DoEnter()
    {
        StatusEvent().InvokeOnMove();
        ResetMoveSpeedFromVelocity(moveSpeed_);
        footstep_ = {};
    }

    void SwordManAvatarWalkState::DoFixedUpdate()
    {
        MoveForward(moveSpeed_, Status().GetWalkSpeed(), Resources().WalkAccelerationTime_secs(), Resources().WalkDecelerationTime_secs());
    }

    void SwordManAvatarWalkState::DoUpdate()
    {
        TryEmitFootstep(footstep_, Resources().WalkFootstepSounds());

        UpdateLockOn();
        UpdateItemPouchInput();
        UpdateTransitions();
    }

    void SwordManAvatarWalkState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        visitor.Automatic(SwordManAvatarStateType::InjuredWalk, Status().IsInjured());
        visitor.Automatic(SwordManAvatarStateType::Hurt, Status().IsDamaged());
        visitor.OnInput(SwordManAvatarStateType::Idle, SwordManAvatarInput::Move, PlayerAvatarInputPhase::NotHolding, true);
        visitor.OnInput(Status().IsInjured() ? SwordManAvatarStateType::InjuredRun : SwordManAvatarStateType::Run,
                        SwordManAvatarInput::Run, PlayerAvatarInputPhase::Holding, Status().CanRun());
        visitor.OnInput(SwordManAvatarStateType::Jump, SwordManAvatarInput::Jump, PlayerAvatarInputPhase::Pressed, Status().CanJump());
        visitor.OnInput(SwordManAvatarStateType::AvoidRolling, SwordManAvatarInput::AvoidRolling, PlayerAvatarInputPhase::Pressed, Status().CanAvoidRolling());
        visitor.Action(SwordManAvatarStateAction::Move, true);
        VisitLockOnAction(visitor);
        visitor.Action(SwordManAvatarStateAction::CycleItem, true);
        visitor.Action(SwordManAvatarStateAction::UseItem, Status().Pouch().CanUseSelected());
        visitor.Action(SwordManAvatarStateAction::OpenMenu, true);
        visitor.OnInput(SwordManAvatarStateType::NormalAttack, SwordManAvatarInput::NormalAttack, PlayerAvatarInputPhase::Pressed, true);
        visitor.Automatic(SwordManAvatarStateType::UseCanon, Conditions().CanUseCannon());
        visitor.Automatic(SwordManAvatarStateType::Floating, !Conditions().IsGround());
    }

    void SwordManAvatarWalkState::DoExit()
    {

    }
}
