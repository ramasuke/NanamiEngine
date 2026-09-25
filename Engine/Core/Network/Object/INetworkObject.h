#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Core::Network
{
    /** Network上で共有しているオブジェクト */
    class NANAMI_API INetworkObject
    {
    public:
        virtual ~INetworkObject() = default;
    };
}