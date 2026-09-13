#include "NetworkObjectInstanceRegistry.h"

#include "../../../../Module/Component/ComponentBase.h"
#include "../../../../Module/GameObject/Interface/IGameObject.h"
#include "../../../../Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../Tickable/INetworkTickable.h"

namespace NanamiEngine::Core::Network
{
    void NetworkObjectInstanceRegistry::RegisterWithId(
        const NetworkObjectId id,
        const std::weak_ptr<Module::GameObject::IGameObject>& object,
        const OwnerLeavePolicy policy)
    {
        Entry& entry   = entries_[id.Value()];
        entry.instance = object;
        entry.policy   = policy;
        if (entry.owner == PlayerId::Invalid())
            entry.owner = id.SpawnerId();

        RegisterTickables(object);
    }

    void NetworkObjectInstanceRegistry::Unregister(const NetworkObjectId id)
    {
        entries_.erase(id.Value());
    }

    void NetworkObjectInstanceRegistry::UnregisterObject(const std::shared_ptr<Module::GameObject::IGameObject>& object)
    {
        for (auto it = entries_.begin(); it != entries_.end();)
        {
            if (it->second.instance.lock() == object)
                it = entries_.erase(it);
            else
                ++it;
        }
    }

    std::weak_ptr<Module::GameObject::IGameObject> NetworkObjectInstanceRegistry::Find(
        const NetworkObjectId id) const
    {
        const auto it = entries_.find(id.Value());
        if (it == entries_.end())
            return {};
        return it->second.instance;
    }

    PlayerId NetworkObjectInstanceRegistry::OwnerOf(const NetworkObjectId id) const
    {
        const auto it = entries_.find(id.Value());
        if (it == entries_.end())
            return PlayerId::Invalid();
        return it->second.owner;
    }

    void NetworkObjectInstanceRegistry::SetOwner(const NetworkObjectId id, const PlayerId owner)
    {
        entries_[id.Value()].owner = owner;
    }

    std::vector<OwnedEntry> NetworkObjectInstanceRegistry::CollectOwnedBy(const PlayerId owner) const
    {
        std::vector<OwnedEntry> result;
        for (const auto& [rawId, entry] : entries_)
        {
            if (entry.owner == owner && !entry.instance.expired())
                result.push_back({ NetworkObjectId(rawId), entry.policy });
        }
        return result;
    }

    std::vector<OwnerOverride> NetworkObjectInstanceRegistry::CollectOwnerOverrides() const
    {
        std::vector<OwnerOverride> result;
        for (const auto& [rawId, entry] : entries_)
        {
            const NetworkObjectId id(rawId);
            if (!entry.instance.expired() && entry.owner != id.SpawnerId())
                result.push_back({ id, entry.owner });
        }
        return result;
    }

    void NetworkObjectInstanceRegistry::RegisterTickables(
        const std::weak_ptr<Module::GameObject::IGameObject>& weakObject)
    {
        const auto gameObject = weakObject.lock();
        if (!gameObject)
            return;

        for (const auto& tickable : gameObject->Components().Catches<INetworkTickable>())
            tickableRegistry_.Register(tickable);
    }
}
