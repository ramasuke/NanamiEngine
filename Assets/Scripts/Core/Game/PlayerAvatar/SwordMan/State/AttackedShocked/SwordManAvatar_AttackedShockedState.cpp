#include "SwordManAvatar_AttackedShockedState.h"

#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoEnter()
{

}

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoFixedUpdate()
{
    //Change State
    if (Status().AttackedShockedStateDuration_secs() <= During_secs())
    {
        if (Status().IsOnDisableReinforceMode())
            OnChangeState(SwordManAvatarStateType::OnDisableReinforce);
        if (!Input().Move().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Idle);
        if (Input().Move().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Walk);
        if (Input().Run().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Run);
        if (Input().Jump().IsPressed())
            OnChangeState(SwordManAvatarStateType::Jump);
        if (Input().AvoidRolling().IsPressed())
            OnChangeState(SwordManAvatarStateType::AvoidRolling);
        if (Input().NormalAttack().IsPressed())
            OnChangeState(SwordManAvatarStateType::NormalAttack);
        if (!Conditions().IsGround())
            OnChangeState(SwordManAvatarStateType::Floating);
    }
}

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoUpdate()
{

}

void GameCore::PlayerAvatar::SwordMan::State::AttackedShockedState::DoExit()
{

}
