#include "MagicCasterAvatarFloatingState.h"

#include "../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::MagicCaster::State::FloatingState::DoEnter()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::FloatingState::DoFixedUpdate()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::FloatingState::DoUpdate()
{
    if (!Conditions().IsGround())
        return;

    if (Status().IsDamaged())
    {
        OnChangeState(MagicCasterAvatarStateType::Hurt);
        return;
    }
    if (Input().Move().IsUpdatePressed())
    {
        OnChangeState((Input().Run().IsUpdatePressed() && Status().CanRun())
            ? MagicCasterAvatarStateType::Run
            : MagicCasterAvatarStateType::Walk);
        return;
    }
    OnChangeState(MagicCasterAvatarStateType::Idle);
}

void GameCore::PlayerAvatar::MagicCaster::State::FloatingState::DoExit()
{
}
