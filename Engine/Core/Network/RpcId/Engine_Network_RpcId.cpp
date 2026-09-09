#include "Engine_Network_RpcId.h"

namespace NanamiEngine::Core::Network
{
    std::string RpcId::ToString() const
    {
        return std::to_string(value_);
    }
}
