#include "SwordManAvatar_HurtState.h"

#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../Death/SwordManAvatar_DeathState.h"
#include "../Idle/SwordManAvatarIdleState.h"

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoEnter()
{
    Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
    Status().ApplyDamage();
    
    if (Status().IsDeath())
        OnChangeState<DeathState>();
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoUpdate()
{
    Status().DiscardDamage();
    if (Status().DamageStateDuration_secs() <= During_secs())
    {
        OnChangeState<SwordManAvatarIdleState>();
    }
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoExit()
{
    
}
