#include "Network_SyncParameter_Id.h"

namespace NanamiEngine::Core::Network
{
    ParameterId::ParameterId(const uint64_t id)
        : id_(id)
    {
    }

    std::string ParameterId::ToString() const
    {
        return std::to_string(id_);
    }
}
