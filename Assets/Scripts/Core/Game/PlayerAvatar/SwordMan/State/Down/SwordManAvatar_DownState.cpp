#include "SwordManAvatar_DownState.h"

#include "../../../IPlayerAvatar.h"

void GameCore::PlayerAvatar::SwordMan::State::DownState::DoEnter()
{
    Status().SetDowned(true);
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::SwordMan::State::DownState::DoFixedUpdate()
{
    HoldHorizontalVelocity();
}

void GameCore::PlayerAvatar::SwordMan::State::DownState::DoUpdate()
{
    bool allDown = true;
    for (const auto& weakAvatar : IPlayerAvatar::PlayerAvatars())
    {
        const auto avatar = weakAvatar.lock();
        if (!avatar)
            continue;

        if (!avatar->PlayerStatus().IsDowned() && !avatar->PlayerStatus().IsDeath())
        {
            allDown = false;
            break;
        }
    }

    if (allDown)
    {
        OnChangeState(SwordManAvatarStateType::Death);
        return;
    }

    if (During_secs() >= Status().DownStateDuration_secs())
        OnChangeState(SwordManAvatarStateType::Death);
}

void GameCore::PlayerAvatar::SwordMan::State::DownState::DoExit()
{
    Status().SetDowned(false);
}
