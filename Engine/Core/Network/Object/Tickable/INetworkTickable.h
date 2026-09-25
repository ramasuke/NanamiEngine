#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Core::Network
{
    class NANAMI_API INetworkTickable
    {
    public:
        virtual ~INetworkTickable() = default;
        virtual void NetworkedTick() = 0;
    };
}
