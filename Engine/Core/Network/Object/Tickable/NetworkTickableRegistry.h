#pragma once
#include <unordered_map>
#include "INetworkTickableRegistry.h"
#include "../../../../Module/Guid/Hash/GuidHash.h"

namespace NanamiEngine::Core::Network
{
    class NetworkTickableRegistry final : public INetworkTickableRegistry
    {
    public:
        Guid Register(std::weak_ptr<INetworkTickable> tickable) override;
        void Unregister(const Guid& guid) override;
        void TickAll() override;

    private:
        std::unordered_map<Guid, std::weak_ptr<INetworkTickable>, GuidHash> tickables_;
    };
}
