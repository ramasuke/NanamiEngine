#include "Engine_Network_NetworkComponent.h"

#include "../../Engine_Network_NetworkRunner.h"
#include "../../../Serialization/Engine_Module_SerializationRegistration.h"

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

        return NetworkRunner().IsLocallyOwned(objectId_);
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

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(Network::NetworkComponent);
#pragma endregion
