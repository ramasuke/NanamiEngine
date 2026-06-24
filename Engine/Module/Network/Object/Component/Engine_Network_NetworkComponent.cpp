#include "Engine_Network_NetworkComponent.h"

#include "../../Engine_Network_NetworkRunner.h"

namespace NanamiEngine::Module::Network
{
    void NetworkComponent::NetworkAwake(const NetworkObjectId id)
    {
        objectId_ = id;
        localIndex_ = 0;
        for (const auto& obj : networkObjects_)
            obj->NetworkAwake(id, localIndex_);
    }

    void NetworkComponent::NetworkedTick()
    {
        
    }

    bool NetworkComponent::HasStateAuthority() const
    {
        if (objectId_ == NetworkObjectId::Invalid())
            return false;

        return objectId_.IsOwnerBy(NetworkRunner().GetPlayerId());
    }

    NetworkObjectId NetworkComponent::GetNetworkObjectId() const
    {
        return objectId_;
    }

    NetworkRunnerBase& NetworkComponent::NetworkRunner() const
    {
        return NetworkRunnerBase::Instance();
    }
}