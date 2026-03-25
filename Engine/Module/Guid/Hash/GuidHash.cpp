#include "GuidHash.h"

#include "../Guid.h"

size_t GuidHash::operator()(const Guid& guid) const
{
    return std::hash<std::string>()(guid.Value());
}
