#include "SwordManAvatarChargeAttackReleaseState.h"

#include "ext/quaternion_geometric.hpp"
#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarChargeAttackReleaseState::DoEnter()
    {
        StatusEvent().InvokeChargeAttack();
        HoldHorizontalVelocity();
        isAttacked_ = false;
        attackTurn_ = {};
        Status().ConsumeChargeAttackStamina();
    }

    void SwordManAvatarChargeAttackReleaseState::DoFixedUpdate()
    {
        // 跳躍開始から発生（振り下ろし）までの間だけ前方へ踏み込む。In Place のクリップでも跳びかかって見えるようにする
        if (isAttacked_ || During_secs() < Status().ChargeAttackLungeStart_secs())
        {
            HoldHorizontalVelocity();
            return;
        }

        LungeForward(Status().ChargeAttackLungeSpeed());
    }

    void SwordManAvatarChargeAttackReleaseState::DoUpdate()
    {
        if (Status().IsDamaged())
        {
            OnChangeState(SwordManAvatarStateType::Hurt);
            return;
        }

        // 発生前（予備動作中）だけ攻撃対象へ向く
        if (!isAttacked_)
            RotateTowardsAttackTarget(attackTurn_, Status().AttackRotateSmoothTime_secs(), Status().LockOnAttackRotateSpeed());

        TryChargeAttack();

        if (During_secs() > Status().ChargeAttack().Duration_secs())
        {
            ChangeToMoveOrIdle();
        }
    }

    void SwordManAvatarChargeAttackReleaseState::DoExit()
    {
    }

    void SwordManAvatarChargeAttackReleaseState::TryChargeAttack()
    {
        const auto& attackStatus = Status().ChargeAttack();

        if (During_secs() <= attackStatus.OccurrenceDuration_secs())
            return;

        if (isAttacked_)
            return;

        isAttacked_ = true;

        // 踏み込みは振り下ろしの瞬間まで。以降はその場で止める
        HoldHorizontalVelocity();

        const float yaw = glm::eulerAngles(Transform().GetWorldRot()).y;
        const glm::quat yRot = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));

        const bool isHit = NormalAttackArea().TryPhysicsAttack(Player(), BuffedAttackPower(attackStatus.AttackPower()));
        PlayAttackSe(isHit);

        // 壁に阻まれたなら叩きつけそのものが成立しないので、地面の岩も出さない
        if (!isHit && TryBlockAttackByWall(NormalAttackArea()))
            return;

        // 空振りでも叩きつけた地面から岩を突き出す（高さは足元に合わせる）
        if (Resources().HasChargeImpactParticlePrefab())
        {
            const glm::vec3 areaPos = NormalAttackArea().Transform().GetWorldPos();
            const glm::vec3 impactPos(areaPos.x, Transform().GetWorldPos().y, areaPos.z);
            NanamiEngine::Scene::GameObject::Instantiate(Resources().ChargeImpactParticlePrefab(), impactPos, yRot);
        }

        if (!isHit)
            return;

        const auto& hitFeel = Status().ChargeHitFeel();
        NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(hitFeel.ShakeIntensity(), hitFeel.ShakeDuration_secs());

        const auto particle = NanamiEngine::Scene::GameObject::Instantiate(Resources().NormalAttackParticlePrefab(), NormalAttackArea().Transform().GetWorldPos(), yRot);
        if (const auto particleObject = particle.lock())
            particleObject->Transform().SetLocalScale(glm::vec3(hitFeel.ParticleScale()));
        DealDamageText(NormalAttackArea(), BuffedAttackPower(attackStatus.AttackPower()));
        ShakeHitTargets(NormalAttackArea(), hitFeel);
    }

    void SwordManAvatarChargeAttackReleaseState::ChangeToMoveOrIdle()
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
