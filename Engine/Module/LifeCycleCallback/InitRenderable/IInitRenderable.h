#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class NANAMI_API IInitRenderable : public virtual Object::IObject
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
        
        virtual ~IInitRenderable() = default;
        virtual void InitRenderer() = 0;
    };
}
