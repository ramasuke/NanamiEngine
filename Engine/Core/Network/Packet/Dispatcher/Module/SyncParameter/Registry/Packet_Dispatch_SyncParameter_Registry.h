#pragma once
#include "../../../../../Object/SyncParameter/Id/Network_SyncParameter_Id.h"

namespace NanamiEngine::Core::Network
{
    class INetworkSyncParameter;
}

namespace NanamiEngine::Core::Network
{
    class SyncParameterRegistry final
    {
    public:
        SyncParameterRegistry() = default;
        void Register(INetworkSyncParameter& parameter, ParameterId id);
        void DeRegister(ParameterId id);
        [[nodiscard]] INetworkSyncParameter* Find(ParameterId id);

    private:
        std::unordered_map<ParameterId, std::reference_wrapper<INetworkSyncParameter>> parameters_;
    };
}
