#pragma once
#include "../SyncParameter/Network_SyncParameter.h"

namespace NanamiEngine::Core::Network
{
    template<typename T>
    using SyncParam = std::shared_ptr<SyncParameter<T>>;
    
    class SyncParamFactory final
    {
    public:
        template <typename T>
        static SyncParam<T> Create(NetworkObjectBase* owner, T defaultValue)
        {
            auto ptr = std::make_shared<SyncParameter<T>>(std::move(defaultValue));
            owner->networkObjects_.push_back(ptr);
            return ptr;
        }
        
        template <typename T>
        static void Register(NetworkObjectBase* owner, const SyncParam<T>& param)
        {
            owner->networkObjects_.push_back(param);
        }
    };
}
