#include "MagicCasterAvatarIdleState.h"

#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoEnter()
{
    ChangeCameraByLockOn();
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
    FaceAimTarget();
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoUpdate()
{
    UpdateLockOn();
    UpdateTransitions();
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::VisitTransitions(
    IMagicCasterAvatarTransitionVisitor& visitor) const
{
    visitor.Automatic(MagicCasterAvatarStateType::Hurt, Status().IsDamaged());
    visitor.Automatic(MagicCasterAvatarStateType::Floating, !Conditions().IsGround());
    visitor.OnInput(MagicCasterAvatarStateType::Jump, MagicCasterAvatarInput::Jump, PlayerAvatarInputPhase::Pressed, Status().CanJump());
    visitor.Cast(CanCastBasicSpell());
    visitor.OnInput(MagicCasterAvatarStateType::Chatting, MagicCasterAvatarInput::Chat, PlayerAvatarInputPhase::Pressed, Conditions().IsChattable());
    visitor.OnInput(MagicCasterAvatarStateType::Walk, MagicCasterAvatarInput::Move, PlayerAvatarInputPhase::Holding, true);
    VisitLockOnAction(visitor);
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoExit()
{
}
