#pragma once
#include "Engine_Network_INetworkSystem.h"

namespace NanamiEngine::Core::Network
{
    class NullNetworkSystem final : public INetworkSystem
    {
    public:
        explicit NullNetworkSystem();
        ~NullNetworkSystem() override;
        void Update() override;
        void Send(const Packet& packet) override;
        [[nodiscard]] std::vector<Packet> PollPackets() override;
        [[nodiscard]] Network::PlayerId GetPlayerId() const override { return PlayerId::Invalid(); }
    };
}
