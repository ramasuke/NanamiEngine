#pragma once
#include "Id/Network_SyncParameter_Id.h"

namespace NanamiEngine::Core::Network { class ByteBuffer; }

namespace NanamiEngine::Core::Network
{
    class INetworkSyncParameter
    {
    public:
        virtual ~INetworkSyncParameter() = default;
        [[nodiscard]] virtual ParameterId GetId() const = 0;
        virtual void WriteTo(ByteBuffer& buffer) const = 0;
        virtual void SyncReceiveParam(const ByteBuffer& buffer, size_t& offset) = 0;
    };
}
