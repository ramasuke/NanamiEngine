#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../Core/Object/IObject.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class NANAMI_API ILateUpdatable : public virtual Object::IObject
    {
    public:
        virtual ~ILateUpdatable() = default;
        virtual void OnLateUpdate() = 0;
        
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const { }
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)       { }
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::ILateUpdatable, 0);