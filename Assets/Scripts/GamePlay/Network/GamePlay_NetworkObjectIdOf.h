#pragma once
#include "Engine/Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GamePlay::Network
{
    // NOTE: gameObject のネットワーク上の id。オンラインでない、またはネットワーク生成されていなければ Invalid
    Core::Network::NetworkObjectId NetworkObjectIdOf(GameObject::IGameObject& gameObject);
}
