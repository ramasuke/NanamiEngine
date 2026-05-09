#include "ContactedData.h"

bool NanamiEngine::Module::Physics::ContactKey::operator==(const ContactKey& rhs) const
{
    return (a_ == rhs.a_ && b_ == rhs.b_) || (a_ == rhs.b_ && b_ == rhs.a_);
}

size_t NanamiEngine::Module::Physics::ContactKeyHash::operator()(const ContactKey& k) const
{
    const uint32_t idA = k.a_.GetIndexAndSequenceNumber();
    const uint32_t idB = k.b_.GetIndexAndSequenceNumber();

    const uint32_t minId = (std::min)(idA, idB);
    const uint32_t maxId = (std::max)(idA, idB);

    return std::hash<uint64_t>()(
        static_cast<uint64_t>(minId) << 32 | maxId
    );
}
