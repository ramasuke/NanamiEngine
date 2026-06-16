#include "Packet_Dispatch_SyncTransform.h"

#include "../../../../Object/Registry/INetworkObjectInstanceRegistry.h"
#include "../../../../../../Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Module/GameObject/Transform/Transform.h"

namespace NanamiEngine::Core::Network
{
    void SyncTransformDispatcher::DispatchSendPacket(
        const NetworkObjectId id, const glm::vec3 position, const glm::quat rotation)
    {
        Packet packet = Packet::Create(DefaultPacketType::SyncTransform);
        packet.Data().Write(id);
        packet.Data().Write(position);
        packet.Data().Write(rotation);
        packet.SetDelivery(DeliveryMode::Unreliable);
        SendPacket(packet);
    }

    void SyncTransformDispatcher::OnReceive(const Packet& packet)
    {
        size_t offset = 0;
        const auto networkObjectId = packet.Data().Read<NetworkObjectId>(offset);
        const auto position        = packet.Data().Read<glm::vec3>(offset);
        const auto rotation        = packet.Data().Read<glm::quat>(offset);

        if (networkObjectId.IsOwnerBy(PlayerId()))
            return;

        const auto gameObject = instanceRegistry_.Find(networkObjectId).lock();
        if (!gameObject)
            return;

        gameObject->Transform().SetWorldPos(position);
        gameObject->Transform().SetWorldRot(rotation);
    }
}
