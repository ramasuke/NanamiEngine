#include "NetworkPrefabObjectRegistry.h"

#include "../../../../Module/GameObject/PrefabGameObject/PrefabGameObject.h"

namespace NanamiEngine::Core::Network
{
    void PrefabObjectRegistry::Add(const std::weak_ptr<Module::GameObject::PrefabGameObject>& object)
    {
        if (const auto shared = object.lock())
        {
            assets_[shared->GetGuid()] = object;
        }
    }

    std::weak_ptr<Module::GameObject::PrefabGameObject>
    PrefabObjectRegistry::Catch(const Guid& guid) const
    {
        const auto it = assets_.find(guid);
        if (it == assets_.end())
            return {};

        if (const auto shared = it->second.lock())
            return shared;

        return {};
    }
}
