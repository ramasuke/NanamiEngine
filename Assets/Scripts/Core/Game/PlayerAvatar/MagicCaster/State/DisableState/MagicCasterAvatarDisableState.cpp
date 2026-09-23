#include "MagicCasterAvatarDisableState.h"

#include "Packages/Cinemachine/VirtualCamera/Behaviour/ThirdPerson/ThirdPersonCameraBehaviour.h"

void GameCore::PlayerAvatar::MagicCaster::State::DisableState::DoEnter()
{
    if (ExpiredCamera())
        return;

    // 操作できない間はロックオンを外す。ReleaseLockOn は FollowFromBehind へ戻す
    if (CameraGroup().IsLockedOn())
        CameraGroup().ReleaseLockOn();
    else
        ChangeCamera(CameraGroup().FollowFromBehind());

    if (const auto camera = CameraGroup().FollowFromBehind().lock())
    {
        if (const auto thirdPerson = camera->Components().Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>().lock())
        {
            thirdPerson->SetEnable(false);
            thirdPerson->SetEnableLockMousePos(false);
        }
    }
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
    if (ExpiredCamera())
        return;

    if (const auto camera = CameraGroup().FollowFromBehind().lock())
    {
        if (const auto thirdPerson = camera->Components().Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>().lock())
            thirdPerson->SetEnable(true);
    }
}
