#include "SwordManAvatarNormalAttackState.h"

#include "../../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../GamePlay/Ui/DealDamageTextBillBoard/UI_DealDamageTextBillBoard.h"
#include "../../../../Input/PlayerAvatarInput_void.h"
#include "../../AttackedShocked/SwordManAvatar_AttackedShockedState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarNormalAttackState::DoEnter()
    {
        Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
        currentCombo_ = 0;
        isAttacked_   = false;
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

        if (During_secs() < attackStatus.Duration_secs() && Input().NormalAttack().IsPressed() && isAttacked_)
        {
            currentCombo_++;
            isAttacked_ = false;

            if (currentCombo_ >= static_cast<int>(comboNormalAttack.size()))
            {
                currentCombo_ = static_cast<int>(comboNormalAttack.size()) - 1;
            }
        }

        
        if (isAttacked_)
            return;
        
        isAttacked_ = true;
        GamePlay::Sound::SoundPlayer::PlaySe(Resources().NormalAttackSound(), Transform().GetWorldPos());
        StatusEvent().InvokeComboAttack();

        if (NormalAttackArea().TryPhysicsAttack(Player(), attackStatus.AttackPower()))
        {
            Status().AddEnhancePowerStack(attackStatus.GetEnhance() * NormalAttackArea().AttackTargetCount());
            const float yaw = glm::eulerAngles(Transform().GetWorldRot()).y;
            const glm::quat yRot = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
            Scene::GameObject::Instantiate(Resources().NormalAttackParticlePrefab(), NormalAttackArea().Transform().GetWorldPos(), yRot);
            DealDamageText(attackStatus.AttackPower());
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

    void SwordManAvatarNormalAttackState::DealDamageText(const Damage::PhysicsPower power)
    {
        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        for (const auto& attackTarget : NormalAttackArea().Targets())
        {
            const auto origin    = Transform().GetWorldPos();
            const auto targetPos = attackTarget.GameObject().Transform().GetWorldPos();
            const auto direction = targetPos - origin;

            const auto raycastHit = Physics::Raycast(
                                            origin,
                                            direction,
                                            glm::length(direction),
                                            mask);

            const auto textPos = raycastHit.Hit() ? raycastHit.Position() : targetPos;
            const auto damageText = Scene::GameObject::Instantiate(Resources().DealDamageTextBillBoardPrefab(), textPos);
            damageText.lock()->Components().Catch<GamePlay::Ui::DealDamageTextBillBoard>().lock()->Play(power.Value());
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
            OnChangeState(SwordManAvatarStateType::Walk);
        }
        else
        {
            OnChangeState(SwordManAvatarStateType::Idle);
        }
    }
}
