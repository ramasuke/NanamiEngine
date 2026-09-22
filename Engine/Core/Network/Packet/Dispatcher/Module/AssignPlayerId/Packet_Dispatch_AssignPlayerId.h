#pragma once
#include "../../Packet_Dispatch_PacketDispatcherBase.h"
#include "../../../../../../../Packages/R4/R4.h"

namespace NanamiEngine::Core::Network
{
    class INetworkSystem;
}

namespace NanamiEngine::Core::Network
{
    class ReceivedAssignPlayerId final : public PacketDispatcherBase
    {
    public:
        explicit ReceivedAssignPlayerId(INetworkSystem& networkSystem);
        [[nodiscard]] R4::Observable<R4::Unit> OnAssignedPlayerId() const { return onAssignedPlayerId_.AsObservable(); }
        
        void ReceivePacket(const Packet& packet) override;
        
    private:
        INetworkSystem& networkSystem_;
        R4::Subject<R4::Unit> onAssignedPlayerId_;
    };
}
