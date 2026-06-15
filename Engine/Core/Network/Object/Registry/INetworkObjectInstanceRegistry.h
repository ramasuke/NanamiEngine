#pragma once
#include <memory>
#include "../../ObjectId/Engine_Network_NetworkObjectId.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Core::Network
{
    class INetworkObjectInstanceRegistry
    {
    public:
        virtual ~INetworkObjectInstanceRegistry() = default;

        [[nodiscard]] virtual NetworkObjectId Register(
            const std::weak_ptr<Module::GameObject::IGameObject>& object) = 0;

        virtual void RegisterWithId(
            NetworkObjectId id,
            const std::weak_ptr<Module::GameObject::IGameObject>& object) = 0;

        [[nodiscard]] virtual std::weak_ptr<Module::GameObject::IGameObject>
            Find(NetworkObjectId id) const = 0;
    };
}
