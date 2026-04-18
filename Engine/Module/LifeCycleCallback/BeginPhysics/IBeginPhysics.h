#pragma once
#include "../cereal/include/cereal/cereal.hpp"
#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IBeginPhysics : public virtual Object::IObject
    {
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
        }
        
        virtual ~IBeginPhysics() = default;
        virtual void OnBeginPhysics() = 0;
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IBeginPhysics, 0);