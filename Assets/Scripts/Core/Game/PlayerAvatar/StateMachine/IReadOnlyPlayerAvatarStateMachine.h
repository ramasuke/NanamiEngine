#pragma once

namespace GameCore::PlayerAvatar
{
    template <typename StateTypeT>
    class IReadOnlyPlayerAvatarStateMachine
    {
    public:
        virtual ~IReadOnlyPlayerAvatarStateMachine() = default;
        [[nodiscard]] virtual StateTypeT GetCurrentStateType() const = 0;
    };
}
