#pragma once
#include <memory>
#include "../../../../Module/Guid/Guid.h"
#include "INetworkTickable.h"

namespace NanamiEngine::Core::Network
{
    class INetworkTickableRegistry
    {
    public:
        virtual ~INetworkTickableRegistry() = default;
        virtual Guid Register(std::weak_ptr<INetworkTickable> tickable) = 0;
        virtual void Unregister(const Guid& guid) = 0;
        virtual void TickAll() = 0;
    };
}
