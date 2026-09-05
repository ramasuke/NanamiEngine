#include "SwordManAvatar_WakeUpState.h"

#include "../../../../../../GamePlay/PlayerAvatar/WakeUpArea/WakeUpArea.h"
#include "../../../Wakeable/IPlayerWakeable.h"

namespace GameCore::PlayerAvatar::SwordMan::State
{
    void WakeUpState::DoEnter()
    {
        if (const auto target = WakeUpArea().CatchWakeUpTarget().lock())
            target->RequestWakeUp();

        OnChangeState(SwordManAvatarStateType::Idle);
    }

    void WakeUpState::DoFixedUpdate()
    {

    }

    void WakeUpState::DoUpdate()
    {

    }

    void WakeUpState::DoExit()
    {

    }
}
