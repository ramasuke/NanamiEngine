#include "SwordManAvatarDashAttackState.h"

#include "../../../../../../../../../Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "../../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../../Idle/SwordManAvatarIdleState.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarDashAttackState::DoEnter()
    {
        StatusEvent().InvokeDashAttack();
        isAttacked_ = false;

        Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
    }

    void SwordManAvatarDashAttackState::DoUpdate()
    {
        TryDashAttack();

        if (During_secs() > Status().DashAttack().Duration_secs())
        {
            ChangeToMoveOrIdle();
        }
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
            auto& particle = CatchPlayerInChild<Component::ParticleSystem>(GamePlay::PlayerAvatar::SwordMan::HIT_NORMAL_ATTACK_PARTICLE_NAME);
            particle.Play();
        }

        //GamePlay::Sound::SoundPlayer::PlaySe(DashAttackSound(), Transform().GetWorldPos());
        Status().AddEnhancePowerStack(attackStatus.GetEnhance() * DashAttackArea().AttackTargetCount());
    }

    void SwordManAvatarDashAttackState::ChangeToMoveOrIdle()
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
