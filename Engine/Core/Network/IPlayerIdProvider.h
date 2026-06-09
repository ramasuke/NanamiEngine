#pragma once
#include "PlayerId/PlayerId.h"

namespace NanamiEngine::Core::Network
{
    class IPlayerIdProvider
    {
    public:
        virtual ~IPlayerIdProvider() = default;
        [[nodiscard]] virtual PlayerId GetPlayerId() const = 0;
    };
}
