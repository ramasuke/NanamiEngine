#include "SwordManAvatar_DownState.h"

#include "../../../../../../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../IPlayerAvatar.h"

void GameCore::PlayerAvatar::SwordMan::State::DownState::DoEnter()
{
    Status().SetDowned(true);
    Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
}

void GameCore::PlayerAvatar::SwordMan::State::DownState::DoFixedUpdate()
{
    Physics::SetLinearVelocity(Collider().BodyId(), glm::vec3(0.0f, Physics::GetLinearVelocity(Collider().BodyId()).y, 0.0f));
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
