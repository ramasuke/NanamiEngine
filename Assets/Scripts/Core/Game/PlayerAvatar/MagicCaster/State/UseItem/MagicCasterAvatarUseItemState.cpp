#include "MagicCasterAvatarUseItemState.h"

#include "../../../Input/PlayerAvatarInput_void.h"

namespace GameCore::PlayerAvatar::MagicCaster::State
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
            OnChangeState(MagicCasterAvatarStateType::Hurt);
            return;
        }
        if (action_.IsFinished(During_secs()))
            OnChangeState(Input().Move().IsUpdatePressed() ? MagicCasterAvatarStateType::Walk : MagicCasterAvatarStateType::Idle);
    }

    void UseItemState::DoExit()
    {
        action_.End();
    }
}
