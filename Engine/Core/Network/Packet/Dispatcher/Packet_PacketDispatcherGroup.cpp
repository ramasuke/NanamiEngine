#include "Packet_PacketDispatcherGroup.h"

#include "../../Engine_Network_INetworkSystem.h"

namespace NanamiEngine::Core::Network
{
    DefaultPacketDispatcher::DefaultPacketDispatcher(
        INetworkSystem& networkSystem,
        INetworkObjectInstanceRegistry& instanceRegistry)
        : instanceRegistry_(instanceRegistry)
        , receivedAssignPlayerId_(networkSystem)
        , spawnNetworkObject_(networkSystem, networkSystem, instanceRegistry)
        , syncTransform_(networkSystem, networkSystem, instanceRegistry)
        , syncAnimation_(networkSystem, networkSystem, instanceRegistry)
    {
    }

    std::weak_ptr<Module::GameObject::IGameObject> DefaultPacketDispatcher::FindNetworkObject(const NetworkObjectId id) const
    {
        return instanceRegistry_.Find(id);
    }

    void DefaultPacketDispatcher::Update()
    {
        syncTransform_.Update();
    }

    void DefaultPacketDispatcher::DispatchReceivedPacket(const Packet& packet)
    {
        auto defaultedType = static_cast<DefaultPacketType>(packet.Type());
        switch (defaultedType)
        {
        case DefaultPacketType::AssignPlayerId:
            receivedAssignPlayerId_.ReceivePacket(packet);
            break;
        case DefaultPacketType::SpawnNetworkObject:
            spawnNetworkObject_.ReceivePacket(packet);
            break;
        case DefaultPacketType::SyncTransform:
            syncTransform_.ReceivePacket(packet);
            break;
        case DefaultPacketType::SyncAnimation:
            syncAnimation_.ReceivePacket(packet);
            break;
        }
    }
}
