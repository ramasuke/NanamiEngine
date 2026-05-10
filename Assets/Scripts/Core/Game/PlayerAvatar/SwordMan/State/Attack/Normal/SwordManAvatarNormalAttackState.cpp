#include "SwordManAvatarNormalAttackState.h"

#include "../../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../Input/PlayerAvatarInput_void.h"
#include "../../Idle/SwordManAvatarIdleState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarNormalAttackState::DoEnter()
    {
        Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
        currentCombo_ = 0;
        isAttacked_   = false;
    }

    void SwordManAvatarNormalAttackState::DoUpdate()
    {
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

        // 次コンボ入力受付
        if (During_secs() < attackStatus.Duration_secs() && Input().NormalAttack().IsPressed())
        {
            currentCombo_++;
            isAttacked_ = false;

            if (currentCombo_ >= static_cast<int>(comboNormalAttack.size()))
                currentCombo_ = static_cast<int>(comboNormalAttack.size()) - 1;
        }

        // 攻撃判定発生
        if (isAttacked_)
            return;

        isAttacked_ = true;

        if (NormalAttackArea().TryPhysicsAttack(Player(), attackStatus.AttackPower()))
        {
            auto& particle = CatchPlayerInChild<Component::ParticleSystem>(GamePlay::PlayerAvatar::SwordMan::HIT_NORMAL_ATTACK_PARTICLE_NAME);
            particle.Play();
        }

        GamePlay::Sound::SoundPlayer::PlaySe(NormalAttackSound(), Transform().GetWorldPos());
        Status().AddEnhancePowerStack(attackStatus.GetEnhance() * NormalAttackArea().AttackTargetCount());
        StatusEvent().InvokeComboAttack();
    }

    void SwordManAvatarNormalAttackState::ChangeToMoveOrIdle()
    {
        if (Status().IsDamaged())
        {
            OnChangeState<HurtState>();
        }
        else if (Input().Move().IsUpdatePressed())
        {
            OnChangeState<SwordManAvatarWalkState>();
        }
        else
        {
            OnChangeState<SwordManAvatarIdleState>();
        }
    }
}
