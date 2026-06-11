#pragma once
#include "../../Engine/Module/Namespace/EngineNamespace.h"
#include "../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_Dispatch_PacketDispatcherBase.h"

namespace NanamiEngine::Core::Network
{
    class DefaultPacketDispatcher;
}

namespace GameCore::Network
{
    class CustomDispatcherBase : public Core::Network::PacketDispatcherBase
    {
    public:
        explicit CustomDispatcherBase(
            Core::Network::DefaultPacketDispatcher& defaultDispatchers,
            const Core::Network::IPlayerIdProvider& playerIdProvider,
            Core::Network::IPacketSender& packetSender);
        virtual ~CustomDispatcherBase() = default;

        [[nodiscard]] Core::Network::DefaultPacketDispatcher& DefaultDispatch() const { return defaultDispatchers_; }
        
    private:
        Core::Network::DefaultPacketDispatcher& defaultDispatchers_;
    };
}
