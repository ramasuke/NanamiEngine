#include "MagicCasterAvatarDisableState.h"

void GameCore::PlayerAvatar::MagicCaster::State::DisableState::DoEnter()
{
    if (ExpiredCamera())
        return;

    // 操作できない間はロックオンを外す。ReleaseLockOn は FollowFromBehind へ戻す
    if (CameraGroup().IsLockedOn())
        CameraGroup().ReleaseLockOn();
    else
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
