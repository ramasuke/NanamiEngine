#pragma once
#include "../../Packet_Dispatch_PacketDispatcherBase.h"
#include "../../../../../../../Libs/LibCore/Rx/SerializableSubject/unit/unit.h"
#include "../rxcpp/rx.hpp"

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
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit> OnAssignedPlayerId() const { return onAssignedPlayerId_.get_observable(); }
        
        void ReceivePacket(const Packet& packet) override;
        
    private:
        INetworkSystem& networkSystem_;
        rxcpp::subjects::subject<LibCore::Rx::unit> onAssignedPlayerId_;
    };
}
