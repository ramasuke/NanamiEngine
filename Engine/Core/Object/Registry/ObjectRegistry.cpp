#include "ObjectRegistry.h"

#include <ranges>

#include "../IObject.h"
#include "../../Api/NanamiModule.h"

namespace NanamiEngine::Core::FileSystem
{
    void ObjectRegistry::Add(const std::weak_ptr<Module::Object::IObject>& object)
    {
        if (const auto shared = object.lock())
        {
            assets_[shared->GetGuid()] = object;
        }
    }

    void ObjectRegistry::Remove(const Guid& guid)
    {
        assets_.erase(guid);
    }

    void ObjectRegistry::Unregister(const Guid& guid, const Module::Object::IObject& object)
    {
        const auto it = assets_.find(guid);
        if (it == assets_.end())
            return;

        if (const auto registered = it->second.lock(); !registered || registered.get() == &object)
        {
            assets_.erase(it);
        }
    }

    void ObjectRegistry::RemoveIfExpired(const Guid& guid)
    {
        const auto it = assets_.find(guid);
        if (it == assets_.end())
            return;

        if (it->second.expired())
        {
            assets_.erase(it);
        }
    }

    std::size_t ObjectRegistry::PurgeExpired()
    {
        return std::erase_if(assets_, [](const auto& pair) { return pair.second.expired(); });
    }

    std::size_t ObjectRegistry::CountAliveOfModule(const void* module) const
    {
        std::size_t count = 0;
        for (const auto& weak : assets_ | std::views::values)
        {
            if (const auto shared = weak.lock(); shared && ModuleOfVTable(shared.get()) == module)
                ++count;
        }
        return count;
    }
}

