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

        // デフォルト実装は IsServer() + ServerType で OnServerRelayReceive / OnServerAuthoritativeReceive / OnReceive へ振り分ける。
        // engine 固有のディスパッチャー (AssignPlayerId 等) は引き続きここをオーバーライドして直接処理できる。
        virtual void ReceivePacket(const Packet& packet);

        /** サンドボックスパターン */
        void SendPacket(const Packet& packet) const;
        [[nodiscard]] PrefabObjectRegistry& NetworkObjectRegistry() const;
        [[nodiscard]] PlayerId PlayerId() const;

    protected:
        [[nodiscard]] bool IsServer() const;

        // Relay モード時にサーバーが呼ぶ。デフォルト: SendPacket（broadcast）+ OnReceive。
        virtual void OnServerRelayReceive(const Packet& packet);

        // Authoritative モード時にサーバーが呼ぶ。デフォルト: OnReceive のみ（broadcast しない）。
        // 独自の検証 + 選択的 SendPacket が必要な場合にオーバーライドする。
        virtual void OnServerAuthoritativeReceive(const Packet& packet);

        // クライアント受信 / サーバー共通ゲームロジック。
        // 派生クラスでオーバーライドし、Packetを受け取った際の処理を記述
        virtual void OnReceive(const Packet& packet);

    private:
        const IPlayerIdProvider& playerIdProvider_;
        IPacketSender& packetSender;

        //PacketDispatcher Ctor generate macro
        #define DEFINE_PACKET_DEFAULT_CONSTRUCTOR(DerivedClass) \
        explicit DerivedClass(const IPlayerIdProvider& playerIdProvider, IPacketSender& packetSender) \
        : PacketDispatcherBase(playerIdProvider, packetSender) {}
    };
}
