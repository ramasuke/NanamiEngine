#include "Network_SyncParameter.h"

#include "../../../../Module/Network/Engine_Network_NetworkRunner.h"

namespace NanamiEngine::Core::Network
{
    void RegisterSyncParam(INetworkSyncParameter& assignParam, const ParameterId id)
    {
        if (auto* runner = Module::Network::NetworkRunnerBase::TryGetInstance())
            runner->DefaultDispatcher().SyncParameter().ParameterRegistry().Register(assignParam, id);
    }

    void DeRegisterParamId(const ParameterId id)
    {
        if (auto* runner = Module::Network::NetworkRunnerBase::TryGetInstance())
            runner->DefaultDispatcher().SyncParameter().ParameterRegistry().DeRegister(id);
    }

    void SyncSendParam(const INetworkSyncParameter& param)
    {
        if (auto* runner = Module::Network::NetworkRunnerBase::TryGetInstance())
            runner->DefaultDispatcher().SyncParameter().DispatchSendPacket(param);
    }
}
