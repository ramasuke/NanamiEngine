#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <unordered_map>

#include "INetworkObjectInstanceRegistry.h"
#include "../Tickable/NetworkTickableRegistry.h"

namespace NanamiEngine::Core::Network
{
    class NANAMI_API NetworkObjectInstanceRegistry final : public INetworkObjectInstanceRegistry
    {
    public:
        void RegisterWithId(
            NetworkObjectId id,
            const std::weak_ptr<Module::GameObject::IGameObject>& object,
            OwnerLeavePolicy policy,
            PlayerId owner) override;
        void Unregister(NetworkObjectId id) override;
        void UnregisterObject(const std::shared_ptr<Module::GameObject::IGameObject>& object) override;

        [[nodiscard]] std::weak_ptr<Module::GameObject::IGameObject>
            Find(NetworkObjectId id) const override;

        [[nodiscard]] PlayerId OwnerOf(NetworkObjectId id) const override;
        void SetOwner(NetworkObjectId id, PlayerId owner) override;
        [[nodiscard]] std::vector<OwnedEntry> CollectOwnedBy(PlayerId owner) const override;
        [[nodiscard]] std::vector<ObjectOwner> CollectOwners() const override;

        [[nodiscard]] INetworkTickableRegistry& GetTickableRegistry() { return tickableRegistry_; }

    private:
        struct NANAMI_API Entry
        {
            std::weak_ptr<Module::GameObject::IGameObject> instance;
            PlayerId         owner  = PlayerId::Invalid();
            OwnerLeavePolicy policy = OwnerLeavePolicy::Transfer;
        };

        void RegisterTickables(const std::weak_ptr<Module::GameObject::IGameObject>& weakObject);

        std::unordered_map<uint32_t, Entry> entries_;
        NetworkTickableRegistry tickableRegistry_;
    };
}
