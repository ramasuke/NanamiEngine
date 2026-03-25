#pragma once
#include "../cereal/include/cereal/types/polymorphic.hpp""

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IEnablableAsset
    {
    public:
        virtual ~IEnablableAsset() = default;
        virtual void OnEnableAsset() = 0;
        
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

CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IEnablableAsset, 0);