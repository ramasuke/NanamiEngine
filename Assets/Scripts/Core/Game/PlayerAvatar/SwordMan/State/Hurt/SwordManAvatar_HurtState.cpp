#include "SwordManAvatar_HurtState.h"

#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../Input/PlayerAvatarInput_void.h"

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoEnter()
{
    // 被弾の瞬間にカメラを揺らす(揺れ幅・長さは ShakeCameraBehaviour の設定値)。
    NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera();

    Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
    Status().ApplyDamage();

    if (Status().IsDeath())
        OnChangeState(SwordManAvatarStateType::Down);
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoFixedUpdate()
{
    
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoUpdate()
{
    Status().DiscardDamage();
    
    if (During_secs() >= Status().DamageStateDuration_secs())
    {
        if (!Input().Move().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Idle);
        if (Input().Move().IsUpdatePressed())
            OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk);
        if (Input().Run().IsUpdatePressed() && Status().CanRun())
            OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredRun : SwordManAvatarStateType::Run);
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

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoExit()
{

}
