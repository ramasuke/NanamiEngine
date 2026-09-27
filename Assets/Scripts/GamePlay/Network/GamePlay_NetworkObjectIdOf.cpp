#include "GamePlay_NetworkObjectIdOf.h"

#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"

namespace GamePlay::Network
{
    Core::Network::NetworkObjectId NetworkObjectIdOf(GameObject::IGameObject& gameObject)
    {
        if (!Module::Network::NetworkRunnerBase::TryGetInstance())
            return Core::Network::NetworkObjectId::Invalid();

        const auto networkGameObject = gameObject.Components().Catch<Module::Network::NetworkGameObject>().lock();
        return networkGameObject ? networkGameObject->GetNetworkObjectId() : Core::Network::NetworkObjectId::Invalid();
    }
}
