#pragma once
#include "../NetworkSystem_Packet.h"
#include "../../PlayerId/PlayerId.h"

namespace NanamiEngine::Core::Network
{
    class IPlayerIdProvider;
}

namespace NanamiEngine::Core::Network
{
    class IPacketSender;
}

namespace NanamiEngine::Core::Network
{
    class PrefabObjectRegistry;
}

namespace NanamiEngine::Core::Network
{
    class PacketDispatcherBase
    {
    public:
        explicit PacketDispatcherBase(
            const IPlayerIdProvider& playerIdProvider,
            IPacketSender& packetSender);
        
        virtual ~PacketDispatcherBase() = default;
        virtual void ReceivePacket(const Packet& packet) = 0;

        /** サンドボックスパターン */
        void SendPacket(const Packet& packet) const;
        [[nodiscard]] PrefabObjectRegistry& NetworkObjectRegistry() const;
        [[nodiscard]] PlayerId PlayerId() const;

    private:
        const IPlayerIdProvider& playerIdProvider_;
        IPacketSender& packetSender;

    protected:
        //PacketDispatcher Ctor generate macro 
        #define DEFINE_PACKET_DEFAULT_CONSTRUCTOR(DerivedClass) \
        explicit DerivedClass(const IPlayerIdProvider& playerIdProvider, IPacketSender& packetSender) \
        : PacketDispatcherBase(playerIdProvider, packetSender) {}
    };
}
