#pragma once
#include "Engine/Core/Api/NanamiApi.h"
class Guid;

struct NANAMI_API GuidHash final
{
    size_t operator()(const Guid& guid) const;
};
