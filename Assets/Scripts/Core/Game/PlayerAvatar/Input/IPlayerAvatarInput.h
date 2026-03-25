#pragma once

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarInput
    {
    public:
        virtual ~IPlayerAvatarInput() = default;
        virtual void OnUpdate() = 0;
    };
}
