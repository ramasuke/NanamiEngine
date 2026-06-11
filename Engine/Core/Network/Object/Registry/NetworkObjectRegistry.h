#pragma once
#include <memory>
#include <unordered_map>

#include "../../../../Module/Guid/Guid.h"

namespace NanamiEngine::Module::GameObject
{
    class PrefabGameObject;
}

namespace NanamiEngine::Core::Network
{
    class PrefabObjectRegistry final
    {
    public:
        void Add(const std::weak_ptr<Module::GameObject::PrefabGameObject>& object);

        [[nodiscard]] std::weak_ptr<Module::GameObject::PrefabGameObject> Catch(const Guid& guid) const;

    private:
        std::unordered_map<Guid, std::weak_ptr<Module::GameObject::PrefabGameObject>, GuidHash> assets_;
    };
}
