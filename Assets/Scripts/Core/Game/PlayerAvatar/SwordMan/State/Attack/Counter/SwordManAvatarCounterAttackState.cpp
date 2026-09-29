#include "SwordManAvatarCounterAttackState.h"

#include "ext/quaternion_geometric.hpp"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarCounterAttackState::DoEnter()
    {
        Status().ConsumeCounter();
        HoldHorizontalVelocity();
        isAttacked_ = false;
        attackTurn_ = {};
    }

    void SwordManAvatarCounterAttackState::DoFixedUpdate()
    {
        // 発生の瞬間まで相手へ踏み込む
        if (isAttacked_)
        {
            HoldHorizontalVelocity();
            return;
        }

        LungeForward(Status().CounterHitFeel().LungeSpeed());
    }

    void SwordManAvatarCounterAttackState::DoUpdate()
    {
        if (Status().IsDamaged())
        {
            OnChangeState(SwordManAvatarStateType::Hurt);
            return;
        }

        if (!isAttacked_)
            RotateTowardsAttackTarget(attackTurn_, Status().AttackRotateSmoothTime_secs(), Status().LockOnAttackRotateSpeed());

        TryCounterAttack();

        if (During_secs() > Status().CounterAttack().Duration_secs())
            ChangeToMoveOrIdle();
    }

    void SwordManAvatarCounterAttackState::DoExit()
    {
    }

    void SwordManAvatarCounterAttackState::TryCounterAttack()
    {
        const auto& attackStatus = Status().CounterAttack();
        if (isAttacked_ || During_secs() <= attackStatus.OccurrenceDuration_secs())
            return;

        isAttacked_ = true;
        HoldHorizontalVelocity();

        const auto power = BuffedAttackPower(attackStatus.AttackPower());
        const bool isHit = NormalAttackArea().TryPhysicsAttack(Player(), power);
        PlayAttackSe(isHit, Resources().CounterAttackWhiffSound(), Resources().CounterAttackHitSound());

        const float yaw = glm::eulerAngles(Transform().GetWorldRot()).y;
        const glm::quat yRot = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        if (Resources().HasCounterSlashParticlePrefab())
            SpawnAttackParticle(Resources().CounterSlashParticlePrefab(), Transform().GetWorldPos(), yRot);

        if (!isHit)
        {
            TryBlockAttackByWall(NormalAttackArea());
            return;
        }

        const auto& hitFeel = Status().CounterHitFeel();
        NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(hitFeel.ShakeIntensity(), hitFeel.ShakeDuration_secs());

        const glm::vec3 hitPos = NormalAttackArea().Transform().GetWorldPos();
        SpawnAttackParticle(Resources().NormalAttackParticlePrefab(), hitPos, yRot, hitFeel.ParticleScale());
        if (Resources().HasCounterImpactParticlePrefab())
            SpawnAttackParticle(Resources().CounterImpactParticlePrefab(), hitPos, yRot);
        DealDamageText(NormalAttackArea(), power);
        ShakeHitTargets(NormalAttackArea(), hitFeel);
    }

    void SwordManAvatarCounterAttackState::ChangeToMoveOrIdle()
    {
        if (Input().Move().IsUpdatePressed())
            OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk);
        else
            OnChangeState(SwordManAvatarStateType::Idle);
    }
}
