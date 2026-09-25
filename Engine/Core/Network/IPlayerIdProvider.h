#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "PlayerId/PlayerId.h"

namespace NanamiEngine::Core::Network
{
    class NANAMI_API IPlayerIdProvider
    {
    public:
        virtual ~IPlayerIdProvider() = default;
        [[nodiscard]] virtual PlayerId GetPlayerId() const = 0;
        [[nodiscard]] virtual bool IsServer() const = 0;
    };
}
