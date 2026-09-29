#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "NetworkSystem_Mode.h"
#include "NetworkSystem_HostEndpoint.h"

namespace NanamiEngine::Core::Network
{
    struct NANAMI_API NetworkStartSettings
    {
        Mode         mode = Mode::Client;
        HostEndpoint host; // Client のときだけ使用する
    };
}
