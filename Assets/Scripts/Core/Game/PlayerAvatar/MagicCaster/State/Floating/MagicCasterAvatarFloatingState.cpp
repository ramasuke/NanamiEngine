#include "MagicCasterAvatarFloatingState.h"

#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::MagicCaster::State::FloatingState::DoEnter()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::FloatingState::DoFixedUpdate()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::FloatingState::DoUpdate()
{
    UpdateTransitions();
}

void GameCore::PlayerAvatar::MagicCaster::State::FloatingState::VisitTransitions(
    IMagicCasterAvatarTransitionVisitor& visitor) const
{
    if (!Conditions().IsGround())
        return;

    visitor.Automatic(MagicCasterAvatarStateType::Hurt, Status().IsDamaged());
    const bool isMoving = Input().Move().IsUpdatePressed();
    visitor.OnInput(MagicCasterAvatarStateType::Run, MagicCasterAvatarInput::Run, PlayerAvatarInputPhase::Holding, isMoving && Status().CanRun());
    visitor.OnInput(MagicCasterAvatarStateType::Walk, MagicCasterAvatarInput::Move, PlayerAvatarInputPhase::Holding, true);
    visitor.Automatic(MagicCasterAvatarStateType::Idle, true);
}

void GameCore::PlayerAvatar::MagicCaster::State::FloatingState::DoExit()
{
}
