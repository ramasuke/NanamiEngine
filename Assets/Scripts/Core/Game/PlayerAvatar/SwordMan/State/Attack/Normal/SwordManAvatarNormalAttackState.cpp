#include "SwordManAvatarNormalAttackState.h"

#include "../../../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../../../Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../Input/PlayerAvatarInput_void.h"
#include "../../AttackedShocked/SwordManAvatar_AttackedShockedState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarNormalAttackState::DoEnter()
    {
        Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
        currentCombo_ = 0;
        isAttacked_   = false;
        bufferedAttackTimer_secs_ = 0.0f;
        releasedSinceEnter_ = false;
    }

    void SwordManAvatarNormalAttackState::DoFixedUpdate()
    {
        
    }

    void SwordManAvatarNormalAttackState::DoUpdate()
    {
        if (Status().IsDamaged())
        {
            OnChangeState(SwordManAvatarStateType::Hurt);
            return;
        }

        // 1段目の発生前まで押し続けていたら、ため攻撃の溜めへ移行する（一度でも離したら溜めには入らない）
        if (!Input().NormalAttack().IsUpdatePressed())
            releasedSinceEnter_ = true;
        if (!releasedSinceEnter_ && currentCombo_ == 0 && !isAttacked_ && Status().CanChargeAttack() &&
            During_secs() >= Status().ChargeAttackHoldThreshold_secs())
        {
            OnChangeState(SwordManAvatarStateType::ChargeAttackCharging);
            return;
        }

        // 発生前（予備動作中）だけロックオン対象へ向く
        if (!isAttacked_)
            RotateTowardsLockOnTarget(Status().LockOnAttackRotateSpeed());

        // 入力バッファ: 判定ウィンドウの前後数フレームの押下も拾えるよう、短時間だけ「押した」ことを憶えておく
        if (Input().NormalAttack().IsPressed())
            bufferedAttackTimer_secs_ = Status().ComboInputBufferWindow_secs();
        else if (bufferedAttackTimer_secs_ > 0.0f)
            bufferedAttackTimer_secs_ -= Time::DeltaTime();

        TryComboAttack();

        if (Status().ComboNormalAttack().at(currentCombo_).Duration_secs() <= During_secs())
        {
            ChangeToMoveOrIdle();
        }
        if (During_secs() > Status().ComboNormalAttackStateDuration_secs())
        {
            ChangeToMoveOrIdle();
        }
    }

    void SwordManAvatarNormalAttackState::DoExit()
    {
        
    }

    void SwordManAvatarNormalAttackState::TryComboAttack()
    {
        const auto& comboNormalAttack = Status().ComboNormalAttack();
        if (comboNormalAttack.empty())
            return;

        if (currentCombo_ >= static_cast<int>(comboNormalAttack.size()))
            return;

        const auto& attackStatus = comboNormalAttack[currentCombo_];
        if (During_secs() <= attackStatus.OccurrenceDuration_secs())
            return;

        if (During_secs() < attackStatus.Duration_secs() && bufferedAttackTimer_secs_ > 0.0f && isAttacked_)
        {
            // 最終段では追加入力を無視（同一スイングの再ヒット防止）
            if (currentCombo_ + 1 >= static_cast<int>(comboNormalAttack.size()))
                return;

            bufferedAttackTimer_secs_ = 0.0f; // 消費済みにする（1回の入力で2段以上進めない）
            currentCombo_++;
            isAttacked_ = false; // 次段の予備動作開始。ヒットは次段の OccurrenceDuration 到達時にその段の AttackPower で発生
            return;
        }

        if (isAttacked_)
            return;

        isAttacked_ = true;
        GamePlay::Sound::SoundPlayer::PlaySe(Resources().NormalAttackSound(), Transform().GetWorldPos());
        StatusEvent().InvokeComboAttack();

        if (NormalAttackArea().TryPhysicsAttack(Player(), attackStatus.AttackPower()))
        {
            const auto& hitFeel = Status().ComboHitFeel().at(currentCombo_);
            TriggerHitStop(hitFeel.HitStopDuration_secs(), hitFeel.HitStopTimeScale());
            NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(hitFeel.ShakeIntensity(), hitFeel.ShakeDuration_secs());

            const float yaw = glm::eulerAngles(Transform().GetWorldRot()).y;
            const glm::quat yRot = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
            const auto particle = NanamiEngine::Scene::GameObject::Instantiate(Resources().NormalAttackParticlePrefab(), NormalAttackArea().Transform().GetWorldPos(), yRot);
            if (const auto particleObject = particle.lock())
                particleObject->Transform().SetLocalScale(glm::vec3(hitFeel.ParticleScale()));
            DealDamageText(NormalAttackArea(), attackStatus.AttackPower());
            ShakeHitTargets(NormalAttackArea(), hitFeel);
        }
        else
        {
            const auto direction = NormalAttackArea().Transform().GetWorldPos() - Transform().GetWorldPos();

            Physics::LayerMask mask = Physics::CreateLayerMask();
            Physics::AddLayer(mask, Physics::Layer::Default);
            
            const auto raycastHit = Physics::Raycast(
                                            Transform().GetWorldPos() + glm::vec3(0.0f, 10.0f, 0.0f),
                                            direction,
                                            glm::length(direction),
                                            mask);
            if (raycastHit.Hit())
            {
                OnChangeState(SwordManAvatarStateType::AttackedShocked);
            }
        }
    }

    void SwordManAvatarNormalAttackState::ChangeToMoveOrIdle()
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
