#include "SwordManAvatar_HurtState.h"

#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoEnter()
{
    Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
    Status().ApplyDamage();

    if (Status().IsDeath())
        OnChangeState(SwordManAvatarStateType::Death);
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoFixedUpdate()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoUpdate()
{
    Status().DiscardDamage();
    
    if (During_secs() >= Status().DamageStateDuration_secs())
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
        if (Status().CanReinforce() && Input().OnReinforce().IsPressed())
            OnChangeState(SwordManAvatarStateType::OnEnableReinforce);
        if (Input().NormalAttack().IsPressed())
            OnChangeState(SwordManAvatarStateType::NormalAttack);
        if (!Conditions().IsGround())
            OnChangeState(SwordManAvatarStateType::Floating);
    }
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoExit()
{

}
