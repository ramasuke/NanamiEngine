#pragma once

namespace NanamiEngine::Core::Network
{
    class INetworkTickable
    {
    public:
        virtual ~INetworkTickable() = default;
        virtual void NetworkedTick() = 0;
    };
}
