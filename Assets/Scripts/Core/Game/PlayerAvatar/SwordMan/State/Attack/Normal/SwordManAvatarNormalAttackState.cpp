#include "SwordManAvatarNormalAttackState.h"

#include "../../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
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
        if (Status().IsDamaged())
        {
            OnChangeState<HurtState>();
            return;
        }

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
            {
                currentCombo_ = static_cast<int>(comboNormalAttack.size()) - 1;
            }
            return;
        }

        // 攻撃判定発生
        if (isAttacked_)
            return;

        isAttacked_ = true;
        GamePlay::Sound::SoundPlayer::PlaySe(Resources().NormalAttackSound(), Transform().GetWorldPos());
        StatusEvent().InvokeComboAttack();

        if (NormalAttackArea().TryPhysicsAttack(Player(), attackStatus.AttackPower()))
        {
            Status().AddEnhancePowerStack(attackStatus.GetEnhance() * NormalAttackArea().AttackTargetCount());
            Scene::GameObject::Instantiate(Resources().NormalAttackParticlePrefab(), Transform().GetWorldPos());
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
                OnChangeState<AttackedShockedState>();
            }
        }
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
