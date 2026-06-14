#include "Engine_Network_NetworkObjectId.h"

namespace NanamiEngine::Core::Network
{
    NetworkObjectId::NetworkObjectId(const uint32_t networkObjectId)
        : networkObjectId_(networkObjectId)
    {
    }

    NetworkObjectId NetworkObjectId::Invalid()
    {
        return NetworkObjectId(0);
    }

    std::string NetworkObjectId::ToString() const
    {
        return std::to_string(networkObjectId_);
    }
}
