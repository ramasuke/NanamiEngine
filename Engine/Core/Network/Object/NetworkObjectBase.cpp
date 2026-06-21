#include "NetworkObjectBase.h"

namespace NanamiEngine::Core::Network
{
    NetworkObjectBase::~NetworkObjectBase() = default;

    void NetworkObjectBase::NetworkAwake(const NetworkObjectId id, uint32_t& localIndex)
    {
        for (const auto& obj : networkObjects_)
            obj->NetworkAwake(id, localIndex);
    }
}
