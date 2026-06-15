#pragma once
#include "../glm/fwd.hpp"
#include "../../Packet_Dispatch_PacketDispatcherBase.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace NanamiEngine::Core::Network
{
    class INetworkObjectInstanceRegistry;

    class SpawnNetworkObject final : public PacketDispatcherBase
    {
    public:
        explicit SpawnNetworkObject(
            const IPlayerIdProvider& playerIdProvider,
            IPacketSender& packetSender,
            INetworkObjectInstanceRegistry& instanceRegistry);

        std::shared_ptr<Module::GameObject::IGameObject> DispatchSendPacket(
            Module::Asset::PrefabGameObjectFile& prefabFile,
            glm::vec3 position,
            glm::quat rotation);

    protected:
        void OnReceive(const Packet& packet) override;

    private:
        INetworkObjectInstanceRegistry& instanceRegistry_;
        uint32_t nextNetworkObjectId_ = 1;
    };
}
