#include "SwordManAvatar_DisableState.h"

#include "Engine/Module/Component/Animator/Animator.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/ThirdPerson/ThirdPersonCameraBehaviour.h"

void GameCore::PlayerAvatar::SwordMan::State::DisableState::DoEnter()
{
    if (!ExpiredCamera())
    {
        CameraGroup().FollowFromBehind().lock()->Components().Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>().lock()->SetEnable(false);
        
        CameraGroup()
            .FollowFromBehind().lock()
            ->Components()
            .Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>().lock()
            ->SetEnableLockMousePos(false);
    }
}

void GameCore::PlayerAvatar::SwordMan::State::DisableState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::SwordMan::State::DisableState::DoUpdate()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::DisableState::DoExit()
{
    if (!ExpiredCamera())
    {
        CameraGroup().FollowFromBehind().lock()->Components().Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>().lock()->SetEnable(true);
    }
}

GameCore::PlayerAvatar::SwordMan::AnimationType GameCore::PlayerAvatar::SwordMan::State::DisableState::
AnimationType() const
{
    if (static_cast<enum AnimationType>(Animator().Param<int>("State").Get()) == AnimationType::Walk)
    {
        return AnimationType::Walk;
    }
    return AnimationType::Idle;
}
