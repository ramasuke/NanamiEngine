#pragma once

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
    };
}
