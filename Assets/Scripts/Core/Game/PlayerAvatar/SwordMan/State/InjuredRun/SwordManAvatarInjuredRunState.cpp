#include "SwordManAvatarInjuredRunState.h"

#include "../../../../../../../Data/PlayerAvatar/Resource/Data_SwordManAvatarResource.h"
#include "../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void SwordManAvatarInjuredRunState::DoEnter()
    {
        StatusEvent().InvokeOnRun();
    }

    void SwordManAvatarInjuredRunState::DoFixedUpdate()
    {
        const auto inputMove = Input().Move().ReadValue();
        Actions().ForwardMove(Status().GetRunSpeed() * glm::vec3(inputMove.x, 0.0f, inputMove.y), Status().GetMoveRotateSpeed());
    }

    void SwordManAvatarInjuredRunState::DoUpdate()
    {
        TryEmitFootstep(Resources().RunFootstepContactPhases(), Resources().RunFootstepSounds());

        if (Status().IsDamaged())
            OnChangeState(SwordManAvatarStateType::Hurt);
        if (Status().IsOnDisableReinforceMode())
            OnChangeState(SwordManAvatarStateType::OnDisableReinforce);
        if (!Status().IsInjured())
            OnChangeState(SwordManAvatarStateType::Run);
        if (!Input().Move().IsUpdatePressed())
            OnChangeState(SwordManAvatarStateType::Idle);
        if (!Input().Run().IsUpdatePressed())
            OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk);
        if (Input().Jump().IsPressed())
            OnChangeState(SwordManAvatarStateType::Jump);
        if (Input().AvoidRolling().IsPressed())
            OnChangeState(SwordManAvatarStateType::AvoidRolling);
        if (Input().DashAttack().IsPressed())
            OnChangeState(SwordManAvatarStateType::DashAttack);
        if (Status().CanReinforce() && Input().OnReinforce().IsPressed())
            OnChangeState(SwordManAvatarStateType::OnEnableReinforce);
        if (Conditions().CanUseCannon())
            OnChangeState(SwordManAvatarStateType::UseCanon);
        if (!Conditions().IsGround())
            OnChangeState(SwordManAvatarStateType::Floating);
    }

    void SwordManAvatarInjuredRunState::DoExit()
    {
    }
}
