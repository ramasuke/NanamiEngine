#include "SwordManAvatarJumpAttackAirState.h"

#include "../../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../../../../../GamePlay/Sound/SoundPlayer.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarJumpAttackAirState::DoEnter()
    {
        StatusEvent().InvokeJumpAttack();
        RigidBody().SetLinearVelocity(glm::vec3(0.0f));
        isPlunging_ = false;
    }

    void SwordManAvatarJumpAttackAirState::DoFixedUpdate()
    {
        const bool isPlunging = During_secs() >= Status().JumpAttackWindup_secs();
        if (isPlunging && !isPlunging_ && Resources().HasJumpAttackPlungeSound())
            GamePlay::Sound::SoundPlayer::PlaySe(Resources().JumpAttackPlungeSound(), Transform().GetWorldPos());
        isPlunging_ = isPlunging;

        const float verticalSpeed = isPlunging ? -Status().JumpAttackPlungeSpeed() : 0.0f;
        RigidBody().SetLinearVelocity(glm::vec3(0.0f, verticalSpeed, 0.0f));
    }

    void SwordManAvatarJumpAttackAirState::DoUpdate()
    {
        UpdateTransitions();
    }

    void SwordManAvatarJumpAttackAirState::DoExit()
    {
    }

    void SwordManAvatarJumpAttackAirState::VisitTransitions(ISwordManAvatarTransitionVisitor& visitor) const
    {
        visitor.Automatic(SwordManAvatarStateType::Hurt, Status().IsDamaged());
        visitor.Automatic(SwordManAvatarStateType::JumpAttackLand, Conditions().IsGround());
    }
}
