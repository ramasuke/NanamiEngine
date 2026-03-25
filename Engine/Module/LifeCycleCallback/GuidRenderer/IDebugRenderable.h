#pragma once
#include <cstdint>

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IDebugRenderable
    {
    public:
        virtual ~IDebugRenderable() = default;
        virtual void OnDebugRender() = 0;
        
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
        }
    };
}
