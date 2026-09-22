#include "SwordManAvatarNormalAttackState.h"

#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Packages/Cinemachine/VirtualCamera/Behaviour/Shake/ShakeCameraBehaviour.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarNormalAttackState::DoEnter()
    {
        HoldHorizontalVelocity();
        currentCombo_ = 0;
        isAttacked_   = false;
        bufferedAttackTimer_secs_ = 0.0f;
        releasedSinceEnter_ = false;
        attackTurn_ = {};
    }

    void SwordManAvatarNormalAttackState::DoFixedUpdate()
    {
        // 各段の発生までは自機の向きへ踏み込む。以降はその場に留める
        if (isAttacked_)
        {
            HoldHorizontalVelocity();
            return;
        }

        LungeForward(Status().ComboAttackLungeSpeed(currentCombo_));
    }

    void SwordManAvatarNormalAttackState::DoUpdate()
    {
        if (!Input().NormalAttack().IsUpdatePressed())
            releasedSinceEnter_ = true;
        if (UpdateTransitions())
            return;

        // 発生前に攻撃対象へ向く
        if (!isAttacked_)
            RotateTowardsAttackTarget(attackTurn_, Status().AttackRotateSmoothTime_secs(), Status().LockOnAttackRotateSpeed());

        // 入力バッファ
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

    void SwordManAvatarNormalAttackState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        // アイテム欄は出したままにするが、この State では使えない。宣言しないと大砲と同じ扱いでアイテム欄ごと消えてしまう
        visitor.Action(SwordManAvatarStateAction::CycleItem, false);
        visitor.Action(SwordManAvatarStateAction::UseItem, false);
        visitor.Automatic(SwordManAvatarStateType::Hurt, Status().IsDamaged());
        visitor.Action(SwordManAvatarStateAction::ComboAttack, currentCombo_ + 1 < static_cast<int>(Status().ComboNormalAttack().size()));
        // 1段目の発生前まで押し続けていたら、ため攻撃の溜めへ移行する
        visitor.OnInput(
            SwordManAvatarStateType::ChargeAttackCharging,
            SwordManAvatarInput::NormalAttack,
            PlayerAvatarInputPhase::Holding,
            !releasedSinceEnter_ && currentCombo_ == 0 && !isAttacked_ && Status().CanChargeAttack(),
            During_secs() >= Status().ChargeAttackHoldThreshold_secs());
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

            bufferedAttackTimer_secs_ = 0.0f;
            currentCombo_++;
            attackTurn_ = {};
            isAttacked_ = false;
            return;
        }

        if (isAttacked_)
            return;

        isAttacked_ = true;
        
        HoldHorizontalVelocity();

        StatusEvent().InvokeComboAttack();

        const bool isHit = NormalAttackArea().TryPhysicsAttack(Player(), BuffedAttackPower(attackStatus.AttackPower()));
        PlayComboAttackSe(isHit);

        if (isHit)
        {
            const auto& hitFeel = Status().ComboHitFeel().at(currentCombo_);
            NanamiEngine::CineMachine::Behaviour::ShakeCameraBehaviour::ShakeMainCamera(hitFeel.ShakeIntensity(), hitFeel.ShakeDuration_secs());

            const float yaw = glm::eulerAngles(Transform().GetWorldRot()).y;
            const glm::quat yRot = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
            const auto particle = NanamiEngine::Scene::GameObject::Instantiate(Resources().NormalAttackParticlePrefab(), NormalAttackArea().Transform().GetWorldPos(), yRot);
            if (const auto particleObject = particle.lock())
                particleObject->Transform().SetLocalScale(glm::vec3(hitFeel.ParticleScale()));
            DealDamageText(NormalAttackArea(), BuffedAttackPower(attackStatus.AttackPower()), false);
            ShakeHitTargets(NormalAttackArea(), hitFeel);
        }
        else
        {
            TryBlockAttackByWall(NormalAttackArea());
        }
    }

    void SwordManAvatarNormalAttackState::PlayComboAttackSe(const bool isHit) const
    {
        const auto& comboSounds = isHit
            ? Resources().ComboNormalAttackHitSounds()
            : Resources().ComboNormalAttackWhiffSounds();
        if (currentCombo_ < static_cast<int>(comboSounds.size()))
        {
            if (const auto sound = comboSounds[currentCombo_].get())
            {
                GamePlay::Sound::SoundPlayer::PlaySe(*sound, Transform().GetWorldPos());
                return;
            }
        }
        PlayAttackSe(isHit);
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
