#include "NetworkTickableRegistry.h"
#include <vector>

namespace NanamiEngine::Core::Network
{
    Guid NetworkTickableRegistry::Register(std::weak_ptr<INetworkTickable> tickable)
    {
        Guid guid;
        tickables_.emplace(guid, std::move(tickable));
        return guid;
    }

    void NetworkTickableRegistry::Unregister(const Guid& guid)
    {
        tickables_.erase(guid);
    }

    void NetworkTickableRegistry::TickAll()
    {
        std::vector<Guid> expired;
        for (auto& [guid, tickable] : tickables_)
        {
            if (const auto locked = tickable.lock())
                locked->NetworkedTick();
            else
                expired.push_back(guid);
        }
        for (const auto& guid : expired)
            tickables_.erase(guid);
    }
}
