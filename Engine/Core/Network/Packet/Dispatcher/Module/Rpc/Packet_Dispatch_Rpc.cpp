#include "Packet_Dispatch_Rpc.h"

#include "../../../../RpcId/Engine_Network_RpcId.h"
#include "../../../../../../Module/Network/Rpc/Engine_Network_RpcHandlerRegistry.h"

namespace NanamiEngine::Core::Network
{
    void RpcDispatcher::OnReceive(const Packet& packet)
    {
        size_t offset = 0;
        const auto id = packet.Data().Read<RpcId>(offset);
        Module::Network::RpcHandlerRegistry::Instance().Invoke(id, packet.Data(), offset);
    }
}
