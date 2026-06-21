#include "SwordManAvatarNormalAttackState.h"

#include "../../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../../../../../../GamePlay/Ui/DealDamageTextBillBoard/UI_DealDamageTextBillBoard.h"
#include "../../../../Input/PlayerAvatarInput_void.h"

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

    void SwordManAvatarNormalAttackState::DoUpdate()
    {

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

        // 攻撃判定発生（このコンボでまだ攻撃していない場合のみ）
        if (!isAttacked_)
        {
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
                                                Transform().GetWorldPos() + glm::vec3(0.0f, 0.0f, 0.0f),
                                                direction,
                                                glm::length(direction),
                                                mask);
                if (raycastHit.Hit())
                {
                    OnChangeState(SwordManAvatarStateType::AttackedShocked);
                }
            }
        }

        // 次コンボ入力受付（攻撃済みの場合のみ受け付ける）
        if (isAttacked_ && During_secs() < attackStatus.Duration_secs() && Input().NormalAttack().IsUpdatePressed())
        {
            if (currentCombo_ + 1 < static_cast<int>(comboNormalAttack.size()))
            {
                currentCombo_++;
                isAttacked_ = false;
            }
        }
    }

    void SwordManAvatarNormalAttackState::DealDamageText(const Damage::PhysicsPower power)
    {
        const auto direction = NormalAttackArea().Transform().GetWorldPos() - Transform().GetWorldPos();

        Physics::LayerMask mask = Physics::CreateLayerMask();
        Physics::AddLayer(mask, Physics::Layer::Default);

        const auto raycastHit = Physics::Raycast(
                                        Transform().GetWorldPos() + glm::vec3(0.0f, 0.0f, 0.0f),
                                        direction,
                                        glm::length(direction) * 100.0f,
                                        mask);
        if (raycastHit.Hit())
        {
            const auto damageText = Scene::GameObject::Instantiate(Resources().DealDamageTextBillBoardPrefab(), raycastHit.Position());
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
