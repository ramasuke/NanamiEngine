#pragma once
#include <unordered_map>

#include "INetworkObjectInstanceRegistry.h"

namespace NanamiEngine::Core::Network
{
    class NetworkObjectInstanceRegistry final : public INetworkObjectInstanceRegistry
    {
    public:
        [[nodiscard]] NetworkObjectId Register(
            const std::weak_ptr<Module::GameObject::IGameObject>& object) override;

        void RegisterWithId(
            NetworkObjectId id,
            const std::weak_ptr<Module::GameObject::IGameObject>& object) override;

        [[nodiscard]] std::weak_ptr<Module::GameObject::IGameObject>
            Find(NetworkObjectId id) const override;

    private:
        std::unordered_map<uint32_t, std::weak_ptr<Module::GameObject::IGameObject>> instances_;
        uint32_t nextId_ = 1;
    };
}
