#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <string>

namespace NanamiEngine::Core::Network
{
    struct NANAMI_API HostEndpoint
    {
        std::string   address;
        std::uint16_t port = 0;
    };
}
