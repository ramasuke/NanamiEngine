#include "MagicCasterAvatarRunState.h"

#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::MagicCaster::State::RunState::DoEnter()
{
    footstep_ = {};
}

void GameCore::PlayerAvatar::MagicCaster::State::RunState::DoFixedUpdate()
{
    const auto inputMove = Input().Move().ReadValue();
    Actions().MoveForward(Status().GetRunSpeed() * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());
}

void GameCore::PlayerAvatar::MagicCaster::State::RunState::DoUpdate()
{
    TryEmitFootstep(footstep_);

    UpdateLockOn();
    if (!UpdateItemPouchInput())
        UpdateTransitions();
}

void GameCore::PlayerAvatar::MagicCaster::State::RunState::VisitTransitions(
    IMagicCasterAvatarTransitionVisitor& visitor) const
{
    visitor.Automatic(MagicCasterAvatarStateType::Hurt, Status().IsDamaged());
    visitor.Automatic(MagicCasterAvatarStateType::Floating, !Conditions().IsGround());
    visitor.OnInput(MagicCasterAvatarStateType::Idle, MagicCasterAvatarInput::Move, PlayerAvatarInputPhase::NotHolding, true);
    visitor.Automatic(MagicCasterAvatarStateType::Walk, !Input().Run().IsUpdatePressed() || !Status().CanRun());
    visitor.OnInput(MagicCasterAvatarStateType::Jump, MagicCasterAvatarInput::Jump, PlayerAvatarInputPhase::Pressed, Status().CanJump());
    visitor.OnInput(MagicCasterAvatarStateType::AvoidRolling, MagicCasterAvatarInput::AvoidRolling, PlayerAvatarInputPhase::Pressed, Status().CanAvoidRolling());
    visitor.Cast(CanCastBasicSpell());
    visitor.Action(MagicCasterAvatarStateAction::Move, true);
    VisitLockOnAction(visitor);
    visitor.Action(MagicCasterAvatarStateAction::CycleItem, true);
    visitor.Action(MagicCasterAvatarStateAction::UseItem, Status().Pouch().CanUseSelected());
}

void GameCore::PlayerAvatar::MagicCaster::State::RunState::DoExit()
{
}
