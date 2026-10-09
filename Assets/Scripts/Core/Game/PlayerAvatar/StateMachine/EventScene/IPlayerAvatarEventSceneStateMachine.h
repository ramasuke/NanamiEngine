#pragma once
#include "PlayerAvatarEventSceneStateType.h"

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarEventSceneStateMachine
    {
    public:
        virtual ~IPlayerAvatarEventSceneStateMachine() = default;
        // NOTE: 演出用に動きを差し替える
        virtual void OnChangeState(EventSceneStateType type) = 0;
    };
}
