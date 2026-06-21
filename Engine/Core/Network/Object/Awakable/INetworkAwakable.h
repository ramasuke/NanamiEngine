#pragma once
#include "../../ObjectId/Engine_Network_NetworkObjectId.h"

namespace NanamiEngine::Core::Network
{
    class INetworkAwakable
    {
    public:
        virtual ~INetworkAwakable() = default;
        virtual void NetworkAwake(NetworkObjectId objectId) = 0;
    };
}
