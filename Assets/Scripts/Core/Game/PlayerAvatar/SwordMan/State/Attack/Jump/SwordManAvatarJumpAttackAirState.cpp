#include "SwordManAvatarJumpAttackAirState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarJumpAttackAirState::DoEnter()
    {
        StatusEvent().InvokeJumpAttack();
        RigidBody().SetLinearVelocity(glm::vec3(0.0f));
        attackTurn_ = {};
    }

    void SwordManAvatarJumpAttackAirState::DoFixedUpdate()
    {
        const float verticalSpeed = During_secs() < Status().JumpAttackWindup_secs() ? 0.0f : -Status().JumpAttackPlungeSpeed();
        RigidBody().SetLinearVelocity(glm::vec3(0.0f, verticalSpeed, 0.0f));
    }

    void SwordManAvatarJumpAttackAirState::DoUpdate()
    {
        if (UpdateTransitions())
            return;

        if (During_secs() < Status().JumpAttackWindup_secs())
            RotateTowardsAttackTarget(attackTurn_, Status().AttackRotateSmoothTime_secs(), Status().LockOnAttackRotateSpeed());
    }

    void SwordManAvatarJumpAttackAirState::DoExit()
    {
    }

    void SwordManAvatarJumpAttackAirState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        visitor.Automatic(SwordManAvatarStateType::Hurt, Status().IsDamaged());
        visitor.Automatic(SwordManAvatarStateType::JumpAttackLand, Conditions().IsGround());
    }
}
