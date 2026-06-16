#pragma once
#include "../glm/fwd.hpp"
#include "../../Packet_Dispatch_PacketDispatcherBase.h"
#include "../../../../ObjectId/Engine_Network_NetworkObjectId.h"

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

        std::shared_ptr<Module::GameObject::IGameObject> SpawnAndRegister(
            Module::Asset::PrefabGameObjectFile& prefabFile,
            glm::vec3 position,
            glm::quat rotation);

        std::shared_ptr<Module::GameObject::IGameObject> DispatchSendPacket(
            Module::Asset::PrefabGameObjectFile& prefabFile,
            glm::vec3 position,
            glm::quat rotation);

        NetworkObjectId AllocateIdAndRegister(const std::shared_ptr<Module::GameObject::IGameObject>& gameObject);
        void RegisterWithNetworkId(NetworkObjectId id, const std::shared_ptr<Module::GameObject::IGameObject>& gameObject);

    protected:
        [[nodiscard]] NetworkObjectId CreateNetworkObjectId();
        void OnReceive(const Packet& packet) override;

    private:
        std::shared_ptr<Module::GameObject::IGameObject> SpawnAndRegisterWithId(
            NetworkObjectId assignedId,
            Module::Asset::PrefabGameObjectFile& prefabFile,
            glm::vec3 position,
            glm::quat rotation);

        void ApplyNetworkId(NetworkObjectId id, const std::shared_ptr<Module::GameObject::IGameObject>& gameObject);

        INetworkObjectInstanceRegistry& instanceRegistry_;
        uint32_t nextNetworkObjectId_ = 1;
    };
}
