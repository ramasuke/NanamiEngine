#include "Engine_Network_NetworkTransform.h"

#include "../GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../../../../Network/Engine_Network_NetworkRunner.h"

namespace NanamiEngine::Module::Network
{
    void NetworkTransform::NetworkedTick()
    {
        if (!HasStateAuthority())
            return;

        const auto networkGameObject = Components().Catch<NetworkGameObject>().lock();
        const auto id = networkGameObject->GetNetworkObjectId();

        NetworkRunner().DefaultDispatcher().SyncTransform()
            .DispatchSendPacket(id, Transform().GetWorldPos(), Transform().GetWorldRot());
    }

    void NetworkTransform::OnDrawGui()
    {
    }
}