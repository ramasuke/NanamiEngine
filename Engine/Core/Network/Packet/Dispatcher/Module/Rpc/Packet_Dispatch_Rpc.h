#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../Packet_Dispatch_PacketDispatcherBase.h"

namespace NanamiEngine::Core::Network
{
    /** 汎用RPC(Module::Network::Rpc<Args...>)を受け取り、RpcHandlerRegistryへ委譲する唯一のディスパッチャー。 */
    class NANAMI_API RpcDispatcher final : public PacketDispatcherBase
    {
    public:
        DEFINE_PACKET_DEFAULT_CONSTRUCTOR(RpcDispatcher)

    protected:
        void OnReceive(const Packet& packet) override;
    };
}
