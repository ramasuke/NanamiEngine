#pragma once
#include <vector>
#include <queue>

#include "Engine_Network_INetworkSystem.h"
#include "Packet/NetworkSystem_Packet.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"

struct _ENetHost;
struct _ENetPeer;

namespace NanamiEngine::Core::Network
{
    constexpr auto CONNECT_PORT_ADDRESS = "127.0.0.1";
    constexpr auto PORT_ADDRESS         = 1234;
    constexpr int  MAX_CLIENTS          = 32;
    
    class EnetUDPNetworkSystem final : public INetworkSystem
    {
    public:
        explicit EnetUDPNetworkSystem();
        ~EnetUDPNetworkSystem() override;
        void Update() override;
        void Send(const Packet& packet) override;
        void SendTo(ENetPeer* target, const Packet& packet);
        [[nodiscard]] std::vector<Packet> PollPackets() override;

    private:
        [[nodiscard]] PlayerId GetPlayerId() const override;
        void SetPlayerId(PlayerId playerId) override;

    private:
        _ENetHost* host_ = nullptr;
        _ENetPeer* peer_ = nullptr;

        std::queue<Packet> receivedQueue_;
        PlayerId playerId_ = PlayerId::Invalid();
        
        std::queue<int> availableIds_;
    };
}