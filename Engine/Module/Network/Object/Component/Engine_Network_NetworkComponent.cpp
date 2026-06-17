#include "Engine_Network_NetworkComponent.h"

#include "GameObject/Engine_Network_NetworkGameObject.h"
#include "../../Engine_Network_NetworkRunner.h"

namespace NanamiEngine::Module::Network
{
    bool NetworkComponent::IsSelfSpawned() const
    {
        const auto networkGameObject = Components().Catch<NetworkGameObject>().lock();
        if (!networkGameObject)
            return false;

        const auto id = networkGameObject->GetNetworkObjectId();
        if (id == Core::Network::NetworkObjectId::Invalid())
            return false;

        return id.IsOwnerBy(NetworkRunner().GetPlayerId());
    }

    NetworkRunnerBase& NetworkComponent::NetworkRunner() const
    {
        return NetworkRunnerBase::Instance();
    }
}