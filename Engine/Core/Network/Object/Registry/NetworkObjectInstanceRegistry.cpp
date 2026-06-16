#include "NetworkObjectInstanceRegistry.h"

#include "../../../../Module/Component/ComponentBase.h"
#include "../../../../Module/GameObject/Interface/IGameObject.h"
#include "../../../../Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../Tickable/INetworkTickable.h"

namespace NanamiEngine::Core::Network
{
    NetworkObjectId NetworkObjectInstanceRegistry::Register(
        const std::weak_ptr<Module::GameObject::IGameObject>& object)
    {
        const NetworkObjectId id(nextId_++);
        instances_[id.Value()] = object;
        RegisterTickables(object);
        return id;
    }

    void NetworkObjectInstanceRegistry::RegisterWithId(
        const NetworkObjectId id,
        const std::weak_ptr<Module::GameObject::IGameObject>& object)
    {
        instances_[id.Value()] = object;
        RegisterTickables(object);
    }

    std::weak_ptr<Module::GameObject::IGameObject> NetworkObjectInstanceRegistry::Find(
        const NetworkObjectId id) const
    {
        const auto it = instances_.find(id.Value());
        if (it == instances_.end())
            return {};
        return it->second;
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
