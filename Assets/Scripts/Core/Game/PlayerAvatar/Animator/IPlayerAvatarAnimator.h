#pragma once

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarAnimator
    {
    public:
        virtual ~IPlayerAvatarAnimator() = default;
        virtual void OnDrawGui() = 0;
    };
}
