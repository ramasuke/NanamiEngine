#include "Packet_Dispatch_SyncTransform.h"

#include "../../../../Object/Registry/INetworkObjectInstanceRegistry.h"
#include "../../../../../../Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Module/GameObject/Transform/Transform.h"
#include "../../../../../Application/Time/Time.h"
#include "../../../../../Application/Configuration/Network/ApplicationConfiguration_Network.h"

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

        auto& buffer = snapshotBuffer_[networkObjectId.Value()];
        buffer.push_back({ Time::CurrentTime(), position, rotation });
        if (buffer.size() > kMaxBufferSize)
            buffer.pop_front();
    }

    void SyncTransformDispatcher::Update()
    {
        using Configuration = Application::Configuration::NetworkConfiguration;
        using Network::ConnectionTarget;

        const float sendInterval    = 1.0f / static_cast<float>(Configuration::GetUnreliableSendRate());
        const float latencyEstimate = (Configuration::GetConnectionTarget() == ConnectionTarget::LAN)
                                      ? 0.01f
                                      : 0.0f;
        const float interpolationDelay = sendInterval * 2.0f + latencyEstimate;
        const float renderTime         = Time::CurrentTime() - interpolationDelay;

        for (auto& [id, buffer] : snapshotBuffer_)
        {
            if (buffer.size() < 2)
                continue;

            const auto gameObject = instanceRegistry_.Find(NetworkObjectId(id)).lock();
            if (!gameObject)
                continue;

            const Snapshot* from = nullptr;
            const Snapshot* to   = nullptr;
            for (size_t i = 0; i + 1 < buffer.size(); ++i)
            {
                if (buffer[i].receiveTime <= renderTime && renderTime <= buffer[i + 1].receiveTime)
                {
                    from = &buffer[i];
                    to   = &buffer[i + 1];
                    break;
                }
            }

            if (!from)
            {
                if (renderTime > buffer.back().receiveTime)
                {
                    const auto& latest = buffer.back();
                    gameObject->Transform().SetWorldPos(latest.position);
                    gameObject->Transform().SetWorldRot(latest.rotation);
                }
                continue;
            }

            const float span = to->receiveTime - from->receiveTime;
            const float t    = (span > 0.0f) ? (renderTime - from->receiveTime) / span : 1.0f;

            gameObject->Transform().SetWorldPos(glm::mix(from->position, to->position, t));
            gameObject->Transform().SetWorldRot(glm::slerp(from->rotation, to->rotation, t));
        }
    }
}
