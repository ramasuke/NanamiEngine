#pragma once
#include "../../../../../../Engine/Core/Network/ObjectId/Engine_Network_NetworkObjectId.h"

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarStateMachine
    {
    public:
        virtual ~IPlayerAvatarStateMachine() = default;
        virtual void OnUpdate() = 0;
        virtual void NetworkTick(NanamiEngine::Core::Network::NetworkObjectId id, bool hasStateAuthority) = 0;
        virtual void OnFixedUpdate() = 0;
    };
}
