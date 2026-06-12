#pragma once
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Core::Network
{
    class DefaultPacketDispatcher;
}

namespace NanamiEngine::Core::Network
{
    struct Packet;
}

namespace NanamiEngine::Core::Network
{
    class IPlayerIdProvider;
}

namespace NanamiEngine::Core::Network
{
    class IPacketSender;
}

namespace GameCore::Network
{
    class CustomDispatcherGroup final
    {
    public:
        explicit CustomDispatcherGroup(
            Core::Network::DefaultPacketDispatcher& defaultDispatchers,
            Core::Network::IPacketSender& packetSender,
            Core::Network::IPlayerIdProvider& playerIdProvider);
        
        void DispatchReceivedPacket(const Core::Network::Packet& packet);

    private:
        
    };
}
