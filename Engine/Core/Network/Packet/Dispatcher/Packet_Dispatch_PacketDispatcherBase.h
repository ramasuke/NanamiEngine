#pragma once
#include "Engine/Core/Api/NanamiApi.h"
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
    class NANAMI_API PacketDispatcherBase
    {
    public:
        explicit PacketDispatcherBase(
            const IPlayerIdProvider& playerIdProvider,
            IPacketSender& packetSender);

        virtual ~PacketDispatcherBase() = default;

        // NOTE: IsServer() + ServerType で OnServerRelayReceive / OnServerAuthoritativeReceive / OnReceive へ振り分ける
        virtual void ReceivePacket(const Packet& packet);

    protected:
        // NOTE: 派生クラス向けの補助 (サンドボックスパターン)
        void SendPacket(const Packet& packet) const;
        [[nodiscard]] PrefabObjectRegistry& NetworkObjectRegistry() const;
        [[nodiscard]] PlayerId PlayerId() const;
        [[nodiscard]] bool IsServer() const;

        // NOTE: Relay モード時のサーバー側受信。既定は SendPacket (broadcast) + OnReceive
        virtual void OnServerRelayReceive(const Packet& packet);

        // NOTE: Authoritative モード時のサーバー側受信。既定は OnReceive のみ (broadcast しない)
        virtual void OnServerAuthoritativeReceive(const Packet& packet);

        // NOTE: クライアント受信とサーバー共通のゲームロジック。派生クラスで Packet を受け取った処理を書く
        virtual void OnReceive(const Packet& packet);

    private:
        const IPlayerIdProvider& playerIdProvider_;
        IPacketSender& packetSender;

        // NOTE: 派生クラスの既定コンストラクタを定義するマクロ
        #define DEFINE_PACKET_DEFAULT_CONSTRUCTOR(DerivedClass) \
        explicit DerivedClass(const IPlayerIdProvider& playerIdProvider, IPacketSender& packetSender) \
        : PacketDispatcherBase(playerIdProvider, packetSender) {}
    };
}
