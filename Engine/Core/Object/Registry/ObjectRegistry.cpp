#include "ObjectRegistry.h"

#include "../IObject.h"

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
}

