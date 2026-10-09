#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../ObjectId/Engine_Network_NetworkObjectId.h"

namespace NanamiEngine::Core::Network
{
    class NANAMI_API INetworkAwakable
    {
    public:
        virtual ~INetworkAwakable() = default;
        // NOTE: ネットワーク上の GameObject としての初期化コールバック
        // NOTE: IAwakable の Awake() と Start() より後に呼ばれる
        virtual void NetworkAwake(NetworkObjectId objectId) = 0;
    };
}
