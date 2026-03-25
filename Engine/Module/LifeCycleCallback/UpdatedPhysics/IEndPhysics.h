#pragma once
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IEndPhysics
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
        
        virtual ~IEndPhysics() = default;
        virtual void OnUpdatedPhysics() = 0;
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IEndPhysics, 0);