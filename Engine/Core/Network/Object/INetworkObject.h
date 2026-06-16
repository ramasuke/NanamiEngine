#pragma once

namespace NanamiEngine::Core::Network
{
    /** Network上で共有しているオブジェクト */
    class INetworkObject
    {
    public:
        virtual ~INetworkObject() = default;
    };
}