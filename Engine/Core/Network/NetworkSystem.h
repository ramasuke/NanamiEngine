#pragma once
#include <vector>
#include <queue>
#include "Packet/NetworkSystem_Packet.h"

struct _ENetHost;
struct _ENetPeer;

namespace NanamiEngine::Core::Network
{
    class NetworkSystem
    {
    public:
        explicit NetworkSystem();
        ~NetworkSystem();

        void Update();
        void Send(const Packet& packet);
        std::vector<Packet> PollPackets();

    private:
        _ENetHost* host_ = nullptr;
        _ENetPeer* peer_ = nullptr;

        std::queue<Packet> receivedQueue_;
    };
}