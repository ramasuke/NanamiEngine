#include "SwordManAvatarUseItemState.h"

#include "../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void UseItemState::DoEnter()
    {
        action_.Begin(Status().Pouch());
    }

    void UseItemState::DoFixedUpdate()
    {
        HoldHorizontalVelocity();
        action_.Update(During_secs(), Status().Pouch(), Status(), Context().PlayerAvatarObject());
    }

    void UseItemState::DoUpdate()
    {
        if (Status().IsDamaged())
        {
            OnChangeState(SwordManAvatarStateType::Hurt);
            return;
        }
        if (!action_.IsFinished(During_secs()))
            return;

        if (Input().Move().IsUpdatePressed())
            OnChangeState(Status().IsInjured() ? SwordManAvatarStateType::InjuredWalk : SwordManAvatarStateType::Walk);
        else
            OnChangeState(SwordManAvatarStateType::Idle);
    }

    void UseItemState::DoExit()
    {
        action_.End();
    }
}
