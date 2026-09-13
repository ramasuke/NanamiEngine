#include "SwordManAvatarDashAttackState.h"

#include "ext/quaternion_geometric.hpp"
#include "../../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarDashAttackState::DoEnter()
    {
        StatusEvent().InvokeDashAttack();
        isAttacked_ = false;

        // 予備動作中は自機の向きへ踏み込む
        const glm::vec3 forward = glm::normalize(glm::vec3(Transform().GetWorldRot() * glm::vec3(0.0f, 0.0f, -1.0f)));
        const float currentY = Physics::GetLinearVelocity(Collider().BodyId()).y;
        Physics::SetLinearVelocity(Collider().BodyId(), forward * Status().DashAttackLungeSpeed() + glm::vec3(0.0f, currentY, 0.0f));
    }

    void SwordManAvatarDashAttackState::DoFixedUpdate()
    {
        TryDashAttack();

        if (During_secs() > Status().DashAttack().Duration_secs())
        {
            ChangeToMoveOrIdle();
        }
    }

    void SwordManAvatarDashAttackState::DoUpdate()
    {
        // 発生前（予備動作中）だけロックオン対象へ向く。発生判定は DoFixedUpdate 側の TryDashAttack が行う
        if (!isAttacked_)
            RotateTowardsLockOnTarget(Status().LockOnAttackRotateSpeed());
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
        Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));

        GamePlay::Sound::SoundPlayer::PlaySe(Resources().NormalAttackSound(), Transform().GetWorldPos());

        if (DashAttackArea().TryPhysicsAttack(Player(), attackStatus.AttackPower()))
        {
            const auto& hitFeel = Status().DashHitFeel();
            TriggerHitStop(hitFeel.HitStopDuration_secs(), hitFeel.HitStopTimeScale());
            NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(hitFeel.ShakeIntensity(), hitFeel.ShakeDuration_secs());

            const auto particle = NanamiEngine::Scene::GameObject::Instantiate(Resources().NormalAttackParticlePrefab(), DashAttackArea().Transform().GetWorldPos());
            if (const auto particleObject = particle.lock())
                particleObject->Transform().SetLocalScale(glm::vec3(hitFeel.ParticleScale()));
            DealDamageText(DashAttackArea(), attackStatus.AttackPower());
            ShakeHitTargets(DashAttackArea(), hitFeel);
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
