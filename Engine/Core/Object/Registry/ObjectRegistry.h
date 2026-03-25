#pragma once
#include <memory>
#include <unordered_map>
#include "../../../Module/Guid/Guid.h"

namespace NanamiEngine::Module::Object
{
    class IObject;
}

namespace NanamiEngine::Core::FileSystem
{
    class ObjectRegistry final
    {
    public:
        void Add(const std::weak_ptr<Module::Object::IObject>& object);
        void Remove(const Guid& guid);

        template <typename T>
        std::weak_ptr<T> Catch(const Guid& guid) const;

    private:
        std::unordered_map<Guid, std::weak_ptr<Module::Object::IObject>, GuidHash> assets_;
    };

    template <typename T>
    std::weak_ptr<T> ObjectRegistry::Catch(const Guid& guid) const
    {
        const auto it = assets_.find(guid);
        if (it == assets_.end())
            return {};

        if (const auto shared = it->second.lock())
        {
            // dynamic_pointer_cast が型チェックを行う
            return std::dynamic_pointer_cast<T>(shared);
        }

        return {};
    }
}