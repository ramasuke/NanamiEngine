#include "SwordManAvatarChargeAttackChargingState.h"

#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarChargeAttackChargingState::DoEnter()
    {
        HoldHorizontalVelocity();
        isFullyCharged_ = false;
        attackTurn_ = {};
    }

    void SwordManAvatarChargeAttackChargingState::DoFixedUpdate()
    {
        HoldHorizontalVelocity();
    }

    void SwordManAvatarChargeAttackChargingState::DoUpdate()
    {
        // 溜め中はその場で停止し、攻撃対象へ向き直るだけ
        RotateTowardsAttackTarget(attackTurn_, Status().AttackRotateSmoothTime_secs(), Status().LockOnAttackRotateSpeed());

        if (!isFullyCharged_ && During_secs() >= Status().ChargeAttackMaxCharge_secs())
        {
            isFullyCharged_ = true;
            EmitChargeCompleteCue();
            if (Resources().HasChargeHoldParticlePrefab())
                chargeHoldParticle_ = NanamiEngine::Scene::GameObject::Instantiate(Resources().ChargeHoldParticlePrefab(), Transform().GetWorldPos());
        }

        SustainChargeShake();

        UpdateTransitions();
    }

    void SwordManAvatarChargeAttackChargingState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        // アイテム欄は出したままにするが、この State では使えない。宣言しないと大砲と同じ扱いでアイテム欄ごと消えてしまう
        visitor.Action(SwordManAvatarStateAction::CycleItem, false);
        visitor.Action(SwordManAvatarStateAction::UseItem, false);
        visitor.Automatic(SwordManAvatarStateType::Hurt, Status().IsDamaged());
        // 最大溜めのまま保持し続けた場合は自動で解放する
        visitor.Automatic(SwordManAvatarStateType::ChargeAttackRelease, isFullyCharged_ && During_secs() >= Status().ChargeAttackMaxHold_secs());
        visitor.OnInput(SwordManAvatarStateType::ChargeAttackRelease, SwordManAvatarInput::NormalAttack, PlayerAvatarInputPhase::NotHolding, isFullyCharged_);
        // 溜め切る前に離した場合は通常コンボの1段目として出し直す
        visitor.OnInput(SwordManAvatarStateType::NormalAttack, SwordManAvatarInput::NormalAttack, PlayerAvatarInputPhase::NotHolding, !isFullyCharged_);
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

        NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(
            Resources().ChargeCompleteShakeIntensity(), Resources().ChargeCompleteShakeDuration_secs());
    }

    void SwordManAvatarChargeAttackChargingState::SustainChargeShake() const
    {
        if (isFullyCharged_)
        {
            NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::SustainShakeMainCamera(Resources().ChargedHoldShakeIntensity());
            return;
        }

        // 最大溜めに近づくほど加速度的に強める
        const float progress  = std::clamp(During_secs() / std::max(Status().ChargeAttackMaxCharge_secs(), 0.001f), 0.0f, 1.0f);
        const float intensity = glm::mix(Resources().ChargingShakeIntensityMin(), Resources().ChargingShakeIntensityMax(), progress * progress);
        NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::SustainShakeMainCamera(intensity);
    }
}
