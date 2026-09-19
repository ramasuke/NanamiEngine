#pragma once
#include <vector>

#include "IPacketSender.h"
#include "IPlayerIdProvider.h"
#include "../../Core/Network/Packet/NetworkSystem_Packet.h"
#include "Mode/NetworkSystem_Mode.h"
#include "Object/Registry/INetworkObjectInstanceRegistry.h"

namespace NanamiEngine::Core::Network
{
    class INetworkSystem : public IPacketSender,
                           public IPlayerIdProvider
    {
    public:
        virtual ~INetworkSystem() override = default;
        virtual void Update    () = 0;
        [[nodiscard]] virtual std::vector<Packet> PollPackets() = 0;
        virtual void SetPlayerId(PlayerId playerId) = 0;
        [[nodiscard]] virtual INetworkObjectInstanceRegistry& GetInstanceRegistry() = 0;
        [[nodiscard]] virtual ConnectionState GetConnectionState() const = 0;
        /** ホストとして待ち受けているポート。クライアントは 0 */
        [[nodiscard]] virtual std::uint16_t ListenPort() const = 0;
    };
}
