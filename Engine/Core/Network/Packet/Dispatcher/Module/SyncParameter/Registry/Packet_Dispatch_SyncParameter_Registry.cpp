#include "Packet_Dispatch_SyncParameter_Registry.h"

namespace NanamiEngine::Core::Network
{
    void SyncParameterRegistry::Register(INetworkSyncParameter& parameter, const ParameterId id)
    {
        parameters_.emplace(id, std::ref(parameter));
    }

    void SyncParameterRegistry::DeRegister(const ParameterId id)
    {
        parameters_.erase(id);
    }

    INetworkSyncParameter* SyncParameterRegistry::Find(const ParameterId id)
    {
        const auto it = parameters_.find(id);
        return it != parameters_.end() ? &it->second.get() : nullptr;
    }
}
