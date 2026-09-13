#include "SwordManAvatarChargeAttackChargingState.h"

#include "../../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarChargeAttackChargingState::DoEnter()
    {
        Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
        isFullyCharged_ = false;
    }

    void SwordManAvatarChargeAttackChargingState::DoFixedUpdate()
    {
    }

    void SwordManAvatarChargeAttackChargingState::DoUpdate()
    {
        if (Status().IsDamaged())
        {
            OnChangeState(SwordManAvatarStateType::Hurt);
            return;
        }

        // 溜め中はその場で停止し、ロックオン対象へ向き直るだけ
        RotateTowardsLockOnTarget(Status().LockOnAttackRotateSpeed());

        if (!isFullyCharged_ && During_secs() >= Status().ChargeAttackMaxCharge_secs())
        {
            isFullyCharged_ = true;
            EmitChargeCompleteCue();
            if (Resources().HasChargeHoldParticlePrefab())
                chargeHoldParticle_ = NanamiEngine::Scene::GameObject::Instantiate(Resources().ChargeHoldParticlePrefab(), Transform().GetWorldPos());
        }

        // 最大溜めのまま保持し続けた場合は自動で解放する
        if (isFullyCharged_ && During_secs() >= Status().ChargeAttackMaxHold_secs())
        {
            OnChangeState(SwordManAvatarStateType::ChargeAttackRelease);
            return;
        }

        if (Input().NormalAttack().IsUpdatePressed())
            return;

        // 溜め切る前に離した場合は通常コンボの1段目として出し直す
        OnChangeState(isFullyCharged_ ? SwordManAvatarStateType::ChargeAttackRelease
                                      : SwordManAvatarStateType::NormalAttack);
    }

    void SwordManAvatarChargeAttackChargingState::DoExit()
    {
        if (const auto particle = chargeHoldParticle_.lock())
            particle->OnDestroy();
        chargeHoldParticle_.reset();
    }

    void SwordManAvatarChargeAttackChargingState::EmitChargeCompleteCue() const
    {
        if (Resources().HasChargeCompleteSound())
            GamePlay::Sound::SoundPlayer::PlaySe(Resources().ChargeCompleteSound(), Transform().GetWorldPos());

        if (Resources().HasChargeCompleteParticlePrefab())
            NanamiEngine::Scene::GameObject::Instantiate(Resources().ChargeCompleteParticlePrefab(), Transform().GetWorldPos());
    }
}
