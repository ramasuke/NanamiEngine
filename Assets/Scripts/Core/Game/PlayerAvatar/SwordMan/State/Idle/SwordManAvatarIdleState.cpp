#include "SwordManAvatarIdleState.h"

#include "../../../../../../../../Engine/Module/Physics/Physics_.h"
#include "../../../Input/PlayerAvatarInput_void.h"
#include "../Attack/Normal/SwordManAvatarNormalAttackState.h"
#include "../AvoidRolling/SwordManAvatar_AvoidRolling.h"
#include "../Chatting/SwordManAvatarChattingState.h"
#include "../Floating/FloatingState.h"
#include "../Hurt/SwordManAvatar_HurtState.h"
#include "../Jump/SwordManAvatarJumpState.h"
#include "../OnDisableReinforce/OnDisableReinforceState.h"
#include "../OnEnableReinforce/OnEnableReinforceState.h"
#include "../Walk/SwordManAvatarWalkState.h"

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoEnter()
{
    ChangeCamera(CameraGroup().FollowFromBehind());
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoUpdate()
{
    Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));

    if (Status().IsDamaged())
        OnChangeState<HurtState>();
    if (Status().IsOnDisableReinforceMode())
        OnChangeState<OnDisableReinforceState>();
    if (Input().Move().IsUpdatePressed())
        OnChangeState<SwordManAvatarWalkState>();
    if (Input().Jump().IsPressed())
        OnChangeState<SwordManAvatarJumpState>();
    if (Input().AvoidRolling().IsPressed())
        OnChangeState<AvoidRollingState>();
    if (Input().NormalAttack().IsPressed())
        OnChangeState<SwordManAvatarNormalAttackState>();
    if (Status().CanReinforce() && Input().OnReinforce().IsPressed())
        OnChangeState<OnEnableReinforceState>();
    if (Conditions().IsChattable() && Input().Chat().IsPressed())
        OnChangeState<SwordManAvatarChattingState>();
    if (!Conditions().IsGround())
        OnChangeState<FloatingState>();
}

void GameCore::PlayerAvatar::SwordMan::State::SwordManAvatarIdleState::DoExit()
{
    
}
