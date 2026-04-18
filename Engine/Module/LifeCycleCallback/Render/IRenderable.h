#pragma once
#include <cstdint>
#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IRenderable : public virtual Object::IObject
    {
    public:
        virtual ~IRenderable() = default;
        virtual void OnRender() = 0;

        template <class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template <class Archive> void load(Archive& archive, const std::uint32_t version)       { }
    };
}
