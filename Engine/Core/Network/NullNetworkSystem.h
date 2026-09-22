#pragma once
#include "Engine_Network_INetworkSystem.h"
#include "Object/Registry/NetworkObjectInstanceRegistry.h"

namespace NanamiEngine::Core::Network
{
    /** 通信しない INetworkSystem。何も送らず、何も受け取らない */
    class NullNetworkSystem final : public INetworkSystem
    {
    public:
        void Update() override {}
        void Send(const Packet&) override {}
        void SendTo(PlayerId, const Packet&) override {}
        [[nodiscard]] R4::Observable<PlayerId> OnConnectPlayer() override { return onConnectPlayer_.AsObservable(); }
        [[nodiscard]] std::vector<Packet> PollPackets() override { return {}; }
        void SetPlayerId(PlayerId) override {}
        [[nodiscard]] INetworkObjectInstanceRegistry& GetInstanceRegistry() override { return instanceRegistry_; }
        [[nodiscard]] ConnectionState GetConnectionState() const override { return ConnectionState::Failed; }
        [[nodiscard]] std::uint16_t ListenPort() const override { return 0; }
        [[nodiscard]] PlayerId GetPlayerId() const override { return PlayerId::Invalid(); }
        [[nodiscard]] bool IsServer() const override { return false; }

    private:
        R4::Subject<PlayerId> onConnectPlayer_;
        NetworkObjectInstanceRegistry instanceRegistry_;
    };
}
