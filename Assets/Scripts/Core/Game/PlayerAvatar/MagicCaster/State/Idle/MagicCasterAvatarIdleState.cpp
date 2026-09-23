#include "MagicCasterAvatarIdleState.h"

#include "Packages/Cinemachine/VirtualCamera/Behaviour/ThirdPerson/ThirdPersonCameraBehaviour.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoEnter()
{
    ChangeCameraByLockOn();

    if (ExpiredCamera())
        return;

    if (const auto camera = CameraGroup().FollowFromBehind().lock())
    {
        if (const auto thirdPerson = camera->Components().Catch<CineMachine::Behaviour::ThirdPersonCameraBehaviour>().lock())
            thirdPerson->SetEnableLockMousePos(true);
    }
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
    FaceAimTarget();
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoUpdate()
{
    UpdateLockOn();
    if (!UpdateItemPouchInput())
        UpdateTransitions();
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::VisitTransitions(
    IMagicCasterAvatarTransitionVisitor& visitor) const
{
    visitor.Automatic(MagicCasterAvatarStateType::Hurt, Status().IsDamaged());
    visitor.Automatic(MagicCasterAvatarStateType::Floating, !Conditions().IsGround());
    visitor.OnInput(MagicCasterAvatarStateType::Jump, MagicCasterAvatarInput::Jump, PlayerAvatarInputPhase::Pressed, Status().CanJump());
    visitor.OnInput(MagicCasterAvatarStateType::AvoidRolling, MagicCasterAvatarInput::AvoidRolling, PlayerAvatarInputPhase::Pressed, Status().CanAvoidRolling());
    visitor.Cast(CanCastBasicSpell());
    visitor.OnInput(MagicCasterAvatarStateType::Chatting, MagicCasterAvatarInput::Chat, PlayerAvatarInputPhase::Pressed, Conditions().IsInteractable());
    visitor.OnInput(MagicCasterAvatarStateType::Walk, MagicCasterAvatarInput::Move, PlayerAvatarInputPhase::Holding, true);
    VisitLockOnAction(visitor);
    visitor.Action(MagicCasterAvatarStateAction::CycleItem, true);
    visitor.Action(MagicCasterAvatarStateAction::UseItem, Status().Pouch().CanUseSelected());
}

void GameCore::PlayerAvatar::MagicCaster::State::IdleState::DoExit()
{
}
