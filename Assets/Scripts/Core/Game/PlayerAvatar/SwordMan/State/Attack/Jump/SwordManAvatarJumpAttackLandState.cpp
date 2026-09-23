#include "SwordManAvatarJumpAttackLandState.h"

#include "ext/quaternion_geometric.hpp"
#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarJumpAttackLandState::DoEnter()
    {
        HoldHorizontalVelocity();
        isAttacked_ = false;
    }

    void SwordManAvatarJumpAttackLandState::DoFixedUpdate()
    {
        HoldHorizontalVelocity();
    }

    void SwordManAvatarJumpAttackLandState::DoUpdate()
    {
        if (UpdateTransitions())
            return;

        TryJumpAttack();
    }

    void SwordManAvatarJumpAttackLandState::DoExit()
    {
    }

    void SwordManAvatarJumpAttackLandState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        visitor.Automatic(SwordManAvatarStateType::Hurt, Status().IsDamaged());
        const bool isFinished = During_secs() > Status().JumpAttack().Duration_secs();
        visitor.Automatic(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk,
                          isFinished && Input().Move().IsUpdatePressed());
        visitor.Automatic(SwordManAvatarStateType::Idle, isFinished);
    }

    void SwordManAvatarJumpAttackLandState::TryJumpAttack()
    {
        const auto& attackStatus = Status().JumpAttack();

        if (isAttacked_ || During_secs() <= attackStatus.OccurrenceDuration_secs())
            return;

        isAttacked_ = true;

        const float yaw = glm::eulerAngles(Transform().GetWorldRot()).y;
        const glm::quat yRot = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));

        if (Resources().HasChargeImpactParticlePrefab())
        {
            const glm::vec3 areaPos = NormalAttackArea().Transform().GetWorldPos();
            const glm::vec3 impactPos(areaPos.x, Transform().GetWorldPos().y, areaPos.z);
            NanamiEngine::Scene::GameObject::Instantiate(Resources().ChargeImpactParticlePrefab(), impactPos, yRot);
        }

        const bool isHit = NormalAttackArea().TryPhysicsAttack(Player(), BuffedAttackPower(attackStatus.AttackPower()));
        PlayAttackSe(isHit, Resources().JumpAttackWhiffSound(), Resources().JumpAttackHitSound());

        if (!isHit)
            return;

        const auto& hitFeel = Status().JumpAttackHitFeel();
        NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(hitFeel.ShakeIntensity(), hitFeel.ShakeDuration_secs());

        const auto particle = NanamiEngine::Scene::GameObject::Instantiate(Resources().NormalAttackParticlePrefab(), NormalAttackArea().Transform().GetWorldPos(), yRot);
        if (const auto particleObject = particle.lock())
            particleObject->Transform().SetLocalScale(glm::vec3(hitFeel.ParticleScale()));
        DealDamageText(NormalAttackArea(), BuffedAttackPower(attackStatus.AttackPower()), false);
        ShakeHitTargets(NormalAttackArea(), hitFeel);
    }
}
