#include "SwordManAvatarInjuredWalkState.h"

#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarInjuredWalkState::DoEnter()
    {
    }

    void SwordManAvatarInjuredWalkState::DoFixedUpdate()
    {
        const auto inputMove = Input().Move().ReadValue();
        Actions().ForwardMove(Status().GetWalkSpeed() * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());
    }

    void SwordManAvatarInjuredWalkState::DoUpdate()
    {
        TryEmitFootstep(Resources().WalkFootstepContactPhases(), Resources().WalkFootstepSounds());

        if (Status().IsDamaged())
            OnChangeState(SwordManAvatarStateType::Hurt);
        if (!Status().IsInjured())
            OnChangeState(SwordManAvatarStateType::Walk);
        if (!Input().Move().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Idle);
        if (Input().Run().IsUpdatePressed() && Status().CanRun())
            OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredRun : SwordManAvatarStateType::Run);
        if (Input().Jump().IsPressed())
            OnChangeState(SwordManAvatarStateType::Jump);
        if (Input().AvoidRolling().IsPressed())
            OnChangeState(SwordManAvatarStateType::AvoidRolling);
        UpdateLockOn();
        if (Input().NormalAttack().IsPressed())
            OnChangeState(SwordManAvatarStateType::NormalAttack);
        if (Conditions().CanUseCannon())
            OnChangeState(SwordManAvatarStateType::UseCanon);
        if (!Conditions().IsGround())
            OnChangeState(SwordManAvatarStateType::Floating);
    }

    void SwordManAvatarInjuredWalkState::DoExit()
    {
    }
}
