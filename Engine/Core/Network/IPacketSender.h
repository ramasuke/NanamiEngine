#pragma once
#include "../../../Libs/LibCore/Rx/SerializableSubject/unit/unit.h"
#include "../rxcpp/rx.hpp"

struct _ENetPeer;
struct _ENetEvent;

namespace NanamiEngine::Core::Network
{
    struct Packet;
}

namespace NanamiEngine::Core::Network
{
    class IPacketSender
    {
    public:
        virtual ~IPacketSender() = default;
        virtual void Send(const Packet& packet) = 0;
        virtual void SendTo(_ENetPeer* target, const Packet& packet) = 0;
        virtual rxcpp::observable<_ENetEvent*> OnConnectPlayer() = 0;
    };
}
