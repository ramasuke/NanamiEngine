#pragma once
#include "../../../Core/Object/IObject.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IPreFixedUpdate : public virtual Object::IObject
    {
    public:
        virtual ~IPreFixedUpdate() = default;
        virtual void OnPreFixedUpdate() = 0;

        template <class Archive>
        void save(Archive&, const std::uint32_t) const {}
        template <class Archive>
        void load(Archive&, const std::uint32_t) {}
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IPreFixedUpdate, 0);
