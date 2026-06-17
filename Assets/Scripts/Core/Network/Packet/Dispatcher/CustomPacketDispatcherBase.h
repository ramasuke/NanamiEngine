#pragma once
#include "../../Engine/Module/Namespace/EngineNamespace.h"
#include "../../../../../../Engine/Core/Network/Packet/Dispatcher/Packet_Dispatch_PacketDispatcherBase.h"
#include "../../../../../../Engine/Core/Network/IPacketSender.h"

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

    protected:
        [[nodiscard]] Core::Network::IPacketSender& PacketSender() const { return packetSender_; }

        // Server/Client 共通のゲームロジック。全 Dispatcher で実装必須。
        // (OnServerRelayReceive / OnServerAuthoritativeReceive のデフォルト実装がここを呼ぶ)
        virtual void OnReceive(const Core::Network::Packet& packet) = 0;

    private:
        Core::Network::DefaultPacketDispatcher& defaultDispatchers_;
        Core::Network::IPacketSender& packetSender_;
    };
}
