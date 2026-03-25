#pragma once
#include <cstdint>

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IInitRenderable
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
