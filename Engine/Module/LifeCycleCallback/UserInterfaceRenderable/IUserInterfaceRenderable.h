#pragma once
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IUserInterfaceRenderable
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
        
        virtual ~IUserInterfaceRenderable() = default;
        virtual void OnUserInterfaceRender() = 0;
        [[nodiscard]] virtual int GetRenderOrder() const = 0;
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IUserInterfaceRenderable, 0);