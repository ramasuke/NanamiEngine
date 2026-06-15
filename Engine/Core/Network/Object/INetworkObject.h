#pragma once
#include "../ObjectId/Engine_Network_NetworkObjectId.h"

namespace NanamiEngine::Core::Network
{
    class INetworkObject
    {
    public:
        virtual ~INetworkObject() = default;

        [[nodiscard]] virtual NetworkObjectId GetNetworkObjectId() const = 0;
        virtual void SetNetworkObjectId(NetworkObjectId id) = 0;
    };
}