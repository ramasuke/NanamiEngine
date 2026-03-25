#pragma once
#include <cstdint>

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IShadowRenderable
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
        
        virtual ~IShadowRenderable() = default;
        virtual void OnShadowRender() = 0;
    };
}
