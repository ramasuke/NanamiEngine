#include "NetworkObjectInstanceRegistry.h"

namespace NanamiEngine::Core::Network
{
    NetworkObjectId NetworkObjectInstanceRegistry::Register(
        const std::weak_ptr<Module::GameObject::IGameObject>& object)
    {
        const NetworkObjectId id(nextId_++);
        instances_[id.Value()] = object;
        return id;
    }

    void NetworkObjectInstanceRegistry::RegisterWithId(
        const NetworkObjectId id,
        const std::weak_ptr<Module::GameObject::IGameObject>& object)
    {
        instances_[id.Value()] = object;
    }

    std::weak_ptr<Module::GameObject::IGameObject> NetworkObjectInstanceRegistry::Find(
        const NetworkObjectId id) const
    {
        const auto it = instances_.find(id.Value());
        if (it == instances_.end())
            return {};
        return it->second;
    }
}
