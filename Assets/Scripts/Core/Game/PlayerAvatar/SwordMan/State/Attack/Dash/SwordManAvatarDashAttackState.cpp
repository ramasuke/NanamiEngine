#include "SwordManAvatarDashAttackState.h"

#include "../../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarDashAttackState::DoEnter()
    {
        StatusEvent().InvokeDashAttack();
        isAttacked_ = false;

        Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
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

        if (DashAttackArea().TryPhysicsAttack(Player(), attackStatus.AttackPower()))
        {
            Status().AddEnhancePowerStack(attackStatus.GetEnhance() * NormalAttackArea().AttackTargetCount());
            Scene::GameObject::Instantiate(Resources().NormalAttackParticlePrefab(), NormalAttackArea().Transform().GetWorldPos());
        }

        //GamePlay::Sound::SoundPlayer::PlaySe(DashAttackSound(), Transform().GetWorldPos());
        Status().AddEnhancePowerStack(attackStatus.GetEnhance() * DashAttackArea().AttackTargetCount());
    }

    void SwordManAvatarDashAttackState::ChangeToMoveOrIdle()
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
