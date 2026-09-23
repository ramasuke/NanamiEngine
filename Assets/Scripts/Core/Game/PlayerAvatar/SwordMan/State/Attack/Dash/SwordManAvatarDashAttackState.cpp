#include "SwordManAvatarDashAttackState.h"

#include "ext/quaternion_geometric.hpp"
#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarDashAttackState::DoEnter()
    {
        StatusEvent().InvokeDashAttack();
        isAttacked_ = false;
        attackTurn_ = {};

        // 予備動作中は自機の向きへ踏み込む
        LungeForward(Status().DashAttackLungeSpeed());
    }

    void SwordManAvatarDashAttackState::DoFixedUpdate()
    {
        TryDashAttack();

        // 踏み込みが終わった後はその場に留める
        if (isAttacked_)
            HoldHorizontalVelocity();

        if (During_secs() > Status().DashAttack().Duration_secs())
        {
            ChangeToMoveOrIdle();
        }
    }

    void SwordManAvatarDashAttackState::DoUpdate()
    {
        // 発生前（予備動作中）だけ攻撃対象へ向く。発生判定は DoFixedUpdate 側の TryDashAttack が行う
        if (!isAttacked_)
            RotateTowardsAttackTarget(attackTurn_, Status().AttackRotateSmoothTime_secs(), Status().LockOnAttackRotateSpeed());
    }

    void SwordManAvatarDashAttackState::DoExit()
    {
    }

    void SwordManAvatarDashAttackState::TryDashAttack()
    {
        const auto& attackStatus = Status().DashAttack();

        if (During_secs() <= attackStatus.OccurrenceDuration_secs())
            return;

        if (isAttacked_)
            return;

        isAttacked_ = true;

        // 踏み込みはヒット判定の瞬間まで。以降はその場で止める(居合い斬りのように踏み込んで止まる)
        HoldHorizontalVelocity();

        const bool isHit = DashAttackArea().TryPhysicsAttack(Player(), BuffedAttackPower(attackStatus.AttackPower()));
        PlayAttackSe(isHit, Resources().DashAttackWhiffSound(), Resources().DashAttackHitSound());

        if (isHit)
        {
            const auto& hitFeel = Status().DashHitFeel();
            NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(hitFeel.ShakeIntensity(), hitFeel.ShakeDuration_secs());

            const auto particle = NanamiEngine::Scene::GameObject::Instantiate(Resources().NormalAttackParticlePrefab(), DashAttackArea().Transform().GetWorldPos());
            if (const auto particleObject = particle.lock())
                particleObject->Transform().SetLocalScale(glm::vec3(hitFeel.ParticleScale()));
            DealDamageText(DashAttackArea(), BuffedAttackPower(attackStatus.AttackPower()), false);
            ShakeHitTargets(DashAttackArea(), hitFeel);
        }
        else
        {
            TryBlockAttackByWall(DashAttackArea());
        }
    }

    void SwordManAvatarDashAttackState::ChangeToMoveOrIdle()
    {
        if (Status().IsDamaged())
        {
            OnChangeState(SwordManAvatarStateType::Hurt);
        }
        else if (Input().Move().IsUpdatePressed())
        {
            OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk);
        }
        else
        {
            OnChangeState(SwordManAvatarStateType::Idle);
        }
    }
}
