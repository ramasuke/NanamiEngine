#include "MagicCasterAvatarDisableState.h"

void GameCore::PlayerAvatar::MagicCaster::State::DisableState::DoEnter()
{
    if (!ExpiredCamera())
        ChangeCamera(CameraGroup().FollowFromBehind());
}

void GameCore::PlayerAvatar::MagicCaster::State::DisableState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::MagicCaster::State::DisableState::DoUpdate()
{
}

void GameCore::PlayerAvatar::MagicCaster::State::DisableState::DoExit()
{
}
