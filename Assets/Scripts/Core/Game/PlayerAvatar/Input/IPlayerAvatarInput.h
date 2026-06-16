#pragma once

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarInput
    {
    public:
        virtual ~IPlayerAvatarInput() = default;
        virtual void OnUpdate() = 0;
        virtual void Enable()   = 0;
        virtual void Disable()  = 0;
    };
}
