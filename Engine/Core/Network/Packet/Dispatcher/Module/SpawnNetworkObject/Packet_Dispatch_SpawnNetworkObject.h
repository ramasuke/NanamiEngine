#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <vector>
#include "../glm/fwd.hpp"
#include "../../Packet_Dispatch_PacketDispatcherBase.h"
#include "../../../../ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../../../Object/Registry/INetworkObjectInstanceRegistry.h"

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
    class NANAMI_API SpawnNetworkObject final : public PacketDispatcherBase
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

        /**
         * policy: 所有者離脱時の扱い。owner: 初期所有者 (受信側は spawn パケットの送信者)
         */
        std::vector<NetworkObjectId> AllocateIdsAndRegister(
            const std::shared_ptr<Module::GameObject::IGameObject>& gameObject,
            OwnerLeavePolicy policy,
            struct PlayerId owner);
        void RegisterWithNetworkIds(
            const std::vector<NetworkObjectId>& ids,
            const std::shared_ptr<Module::GameObject::IGameObject>& gameObject,
            OwnerLeavePolicy policy,
            struct PlayerId owner);
        /** ルート以下のネットワークノードをレジストリから外してから GameObject を破棄する */
        void DespawnAndUnregister(const std::shared_ptr<Module::GameObject::IGameObject>& root);

    protected:
        [[nodiscard]] NetworkObjectId CreateNetworkObjectId();
        void OnReceive(const Packet& packet) override;

    private:
        // ルートと子孫(DFS順)から NetworkGameObject か INetworkAwakable を持つノードを集める
        // WARNING: 送信側と受信側で同じ順・同じ数になる前提で ID を割り当てる
        [[nodiscard]] std::vector<std::shared_ptr<Module::GameObject::IGameObject>> CollectNetworkGameObjects(
            const std::shared_ptr<Module::GameObject::IGameObject>& root) const;
        [[nodiscard]] static bool IsNetworkNode(const std::shared_ptr<Module::GameObject::IGameObject>& gameObject);

        void ApplyNetworkIds(
            const std::vector<NetworkObjectId>& ids,
            const std::shared_ptr<Module::GameObject::IGameObject>& gameObject,
            OwnerLeavePolicy policy,
            struct PlayerId owner);

        INetworkObjectInstanceRegistry& instanceRegistry_;
        uint32_t nextNetworkObjectId_ = 1;
    };
}
