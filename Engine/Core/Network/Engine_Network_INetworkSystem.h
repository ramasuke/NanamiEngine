#pragma once
#include <vector>

#include "IPacketSender.h"
#include "IPlayerIdProvider.h"
#include "../../Core/Network/Packet/NetworkSystem_Packet.h"
#include "Object/Registry/INetworkObjectInstanceRegistry.h"

namespace NanamiEngine::Core::Network
{
    class INetworkSystem : public IPacketSender, public IPlayerIdProvider
    {
    public:
        virtual ~INetworkSystem() = default;
        virtual void Update    () = 0;
        [[nodiscard]] virtual std::vector<Packet> PollPackets() = 0;
        virtual void SetPlayerId(PlayerId playerId) = 0;
        [[nodiscard]] virtual INetworkObjectInstanceRegistry& GetInstanceRegistry() = 0;
    };
}
