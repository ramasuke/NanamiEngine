#include "Engine_Network_NetworkTransform.h"

#include "../GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../Core/Network/Packet/Dispatcher/Packet_PacketDispatcherGroup.h"
#include "../../../../Network/Engine_Network_NetworkRunner.h"
#include "../../../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Network
{
    void NetworkTransform::NetworkedTick()
    {
        if (!HasStateAuthority())
            return;

        // NetworkGameObject を持たない子ノード(NetworkComponent のみ)でも落ちないよう自身の ID を使う
        const auto id = GetNetworkObjectId();

        NetworkRunner()
            .DefaultDispatcher()
            .SyncTransform()
            .DispatchSendPacket(id, Transform().GetWorldPos(), Transform().GetWorldRot());
    }

    void NetworkTransform::OnDrawGui()
    {
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(Network::NetworkTransform);
#pragma endregion
