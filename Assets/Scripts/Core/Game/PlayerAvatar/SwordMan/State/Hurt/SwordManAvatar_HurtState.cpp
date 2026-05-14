#include "SwordManAvatar_HurtState.h"

#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../Input/PlayerAvatarInput_void.h"
#include "../Attack/Normal/SwordManAvatarNormalAttackState.h"
#include "../AvoidRolling/SwordManAvatar_AvoidRolling.h"
#include "../Death/SwordManAvatar_DeathState.h"
#include "../Floating/FloatingState.h"
#include "../Idle/SwordManAvatarIdleState.h"
#include "../Jump/SwordManAvatarJumpState.h"
#include "../OnDisableReinforce/OnDisableReinforceState.h"
#include "../OnEnableReinforce/OnEnableReinforceState.h"
#include "../Run/SwordManAvatarRunState.h"
#include "../Walk/SwordManAvatarWalkState.h"

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
    if (Status().IsOnDisableReinforceMode())
        OnChangeState<OnDisableReinforceState>();
    if (!Input().Move().IsUpdatePressed())
        OnChangeState<SwordManAvatarIdleState>();
    if (Input().Move().IsUpdatePressed())
        OnChangeState<SwordManAvatarWalkState>();
    if (Input().Run().IsUpdatePressed())
        OnChangeState<SwordManAvatarRunState>();
    if (Input().Jump().IsPressed())
        OnChangeState<SwordManAvatarJumpState>();
    if (Input().AvoidRolling().IsPressed())
        OnChangeState<AvoidRollingState>();
    if (Status().CanReinforce() && Input().OnReinforce().IsPressed())
        OnChangeState<OnEnableReinforceState>();
    if (Input().NormalAttack().IsPressed())
        OnChangeState<SwordManAvatarNormalAttackState>();
    if (!Conditions().IsGround())
        OnChangeState<FloatingState>();
}

void GameCore::PlayerAvatar::SwordMan::State::HurtState::DoExit()
{
    
}
