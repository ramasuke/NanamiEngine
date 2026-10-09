#pragma once
#include <functional>
#include <memory>
#include <sstream>
#include <tuple>
#include <type_traits>

#include "Engine_Network_RpcHandlerRegistry.h"
#include "Engine_Network_RpcOwnershipFilter.h"
#include "Engine/Core/Api/NanamiModule.h"
#include "../Engine_Network_NetworkRunner.h"
#include "../../GameObject/Interface/IGameObject.h"
#include "../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../Core/Network/RpcId/Engine_Network_RpcId.h"
#include "../../../Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../../Core/Network/Packet/NetworkSystem_Packet.h"

namespace NanamiEngine::Module::Network
{
    namespace RpcDetail
    {
        template<typename TComponent>
        std::shared_ptr<TComponent> ResolveTarget(
            const Core::Network::ByteBuffer& buffer,
            size_t& offset,
            const RpcOwnershipFilter filter)
        {
            const auto targetId = buffer.Read<Core::Network::NetworkObjectId>(offset);
            const bool isOwner = NetworkRunnerBase::Instance().IsLocallyOwned(targetId);

            if (filter == RpcOwnershipFilter::SkipIfOwner && isOwner)
                return nullptr;
            if (filter == RpcOwnershipFilter::OnlyIfOwner && !isOwner)
                return nullptr;

            const auto object = NetworkRunnerBase::Instance().DefaultDispatcher().FindNetworkObject(targetId).lock();
            if (!object)
                return nullptr;

            return object->Components().Catch<TComponent>().lock();
        }
    }

    template<typename T>
    concept RPCPackable = requires(T value, std::ostream& ofstream, std::istream& ifstream)
    {
        cereal::PortableBinaryOutputArchive(ofstream)(value);
        cereal::PortableBinaryInputArchive(ifstream)(value);
    };

    // NOTE: 対象NetworkObjectIdのコンポーネントのメソッドを1つ呼ぶRPC
    // WARNING: 直接使わず必ずRpcDef経由で使う
    template<typename... Args>
    requires (RPCPackable<Args> && ...)
    class Rpc final
    {
    public:
        template<typename E>
        requires(std::is_enum_v<E> || std::is_integral_v<E>)
        static void Send(
            const E rpcType, const Core::Network::NetworkObjectId target,
            const Core::Network::DeliveryMode delivery, const Args&... args)
        {
            Core::Network::Packet packet = Core::Network::Packet::Create(Core::Network::DefaultPacketType::Rpc);
            packet.Data().Write(Core::Network::RpcId::Create(rpcType));
            packet.Data().Write(target);
            packet.Data().WriteAll(args...);
            packet.SetDelivery(delivery);
            NetworkRunnerBase::Instance().SendNetworkPacket(packet);
        }

        template<typename TComponent, typename E>
        requires(std::is_enum_v<E> || std::is_integral_v<E>)
        static void OnTargeted(
            const E rpcType,
            std::function<void(TComponent&, Args...)> handler,
            const RpcOwnershipFilter filter)
        {
            RpcHandlerRegistry::Instance().Register(Core::Network::RpcId::Create(rpcType),
                [handler, filter](const Core::Network::ByteBuffer& buffer, size_t& offset)
                {
                    auto component = RpcDetail::ResolveTarget<TComponent>(buffer, offset, filter);
                    if (!component)
                        return;
                    
                    std::apply(
                        [&](auto&&... unpacked) { handler(*component, unpacked...); },
                        buffer.ReadAll<Args...>(offset));
                }, NANAMI_CURRENT_MODULE());
        }

        // NOTE: 事前シリアライズ済み/長さプレフィックス無しの生バイト列をそのまま渡す
        template<typename E>
        requires(std::is_enum_v<E> || std::is_integral_v<E>)
        static void SendRaw(
            const E rpcType, const Core::Network::NetworkObjectId target,
            const Core::Network::DeliveryMode delivery, const Core::Network::ByteBuffer& rawPayload)
        {
            static_assert(sizeof...(Args) == 0, "SendRaw is only valid on Rpc<> (no typed Args)");
            Core::Network::Packet packet = Core::Network::Packet::Create(Core::Network::DefaultPacketType::Rpc);
            packet.Data().Write(Core::Network::RpcId::Create(rpcType));
            packet.Data().Write(target);
            packet.Data().Append(rawPayload.Data(), rawPayload.Size());
            packet.SetDelivery(delivery);
            NetworkRunnerBase::Instance().SendNetworkPacket(packet);
        }

        template<typename TComponent, typename E>
        requires(std::is_enum_v<E> || std::is_integral_v<E>)
        static void OnTargetedRaw(
            const E rpcType,
            std::function<void(TComponent&, const Core::Network::ByteBuffer&, size_t&)> handler,
            const RpcOwnershipFilter filter)
        {
            static_assert(sizeof...(Args) == 0, "OnTargetedRaw is only valid on Rpc<> (no typed Args)");
            RpcHandlerRegistry::Instance().Register(Core::Network::RpcId::Create(rpcType),
                [handler, filter](const Core::Network::ByteBuffer& buffer, size_t& offset)
                {
                    auto component = RpcDetail::ResolveTarget<TComponent>(buffer, offset, filter);
                    if (!component)
                        return;
                    handler(*component, buffer, offset);
                }, NANAMI_CURRENT_MODULE());
        }
    };

    // NOTE: RpcTypeとArgsを束ね、送受信側のArgsの食い違いをビルドエラーにする
    template<auto RpcType, typename... Args>
    struct RpcDef final
    {
        static void Send(
            const Core::Network::NetworkObjectId target,
            const Core::Network::DeliveryMode delivery,
            const Args&... args)
        {
            Rpc<Args...>::Send(RpcType, target, delivery, args...);
        }

        template<typename TComponent>
        static void OnTargeted(
            const std::function<void(TComponent&, Args...)>& handler,
            const RpcOwnershipFilter filter)
        {
            Rpc<Args...>::template OnTargeted<TComponent>(RpcType, handler, filter);
        }
    };
}
