#include "NetworkSystem_Packet.h"

#include <assert.h>

namespace NanamiEngine::Core::Network
{
    Packet::Packet(const PacketType type)
        : type_(type)
    {
        assert(type <= (std::numeric_limits<PacketType>::max)());
    }

    ByteBuffer& Packet::Data()
    {
        return data_;
    }

    const ByteBuffer& Packet::Data() const
    {
        return data_;
    }
}
