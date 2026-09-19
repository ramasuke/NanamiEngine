#pragma once
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

        /**
         * policy: 所有者が離脱したときの扱い(プレイヤーアバターは Destroy、敵などそれ以外は Transfer)
         * owner: 初期所有者。送信側は自分、受信側は spawn パケットの送信者
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
        // ルート自身 → 全ての子孫(Transform().GetAllChildren()のDFS順)のうち、
        // NetworkGameObject を持つノード、または INetworkAwakable(NetworkComponent 派生)を1つ以上持つノードを順番に集める。
        // 後者は NetworkGameObject を置かなくても固有の NetworkObjectId を受け取り、RPC の宛先として解決できる
        // (例: 子オブジェクト上の AttackArea)。送信側と受信側は同じプレハブ・同じ手順で数え上げるため ID 数は一致する。
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
