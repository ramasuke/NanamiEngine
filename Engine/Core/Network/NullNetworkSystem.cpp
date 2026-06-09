#include "NullNetworkSystem.h"

namespace NanamiEngine::Core::Network
{
    NullNetworkSystem::NullNetworkSystem()
    {
    }

    NullNetworkSystem::~NullNetworkSystem()
    {
    }

    void NullNetworkSystem::Update()
    {
    }

    void NullNetworkSystem::Send(const Packet& packet)
    {
    }

    std::vector<Packet> NullNetworkSystem::PollPackets()
    {
        return {};
    }
}
