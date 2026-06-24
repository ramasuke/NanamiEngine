#pragma once
#include "../../ObjectId/Engine_Network_NetworkObjectId.h"

namespace NanamiEngine::Core::Network
{
    class INetworkAwakable
    {
    public:
        virtual ~INetworkAwakable() = default;
        /**
         * ネットワーク上のGameObjectとしての初期化Callback
         * NOTE: 必ずIAwakableのAwake()とStart()より後に呼び出される。
         */
        virtual void NetworkAwake(NetworkObjectId objectId) = 0;
    };
}
