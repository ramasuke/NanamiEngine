#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <unordered_map>
#include "INetworkTickableRegistry.h"
#include "../../../../Module/Guid/Hash/GuidHash.h"

namespace NanamiEngine::Core::Network
{
    class NANAMI_API NetworkTickableRegistry final : public INetworkTickableRegistry
    {
    public:
        Guid Register(std::weak_ptr<INetworkTickable> tickable) override;
        void Unregister(const Guid& guid) override;
        void TickAll() override;

    private:
        std::unordered_map<Guid, std::weak_ptr<INetworkTickable>, GuidHash> tickables_;
    };
}
