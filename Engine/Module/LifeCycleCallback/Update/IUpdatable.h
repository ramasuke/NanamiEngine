#pragma once
#include "../../../Core/Object/IObject.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IUpdatable : public virtual Object::IObject
    {
    public:
        virtual ~IUpdatable() = default;
        virtual void OnUpdate() = 0;
        
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const { }
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)       { }
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IUpdatable, 0);