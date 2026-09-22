#include "EnetPeerSupport.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#include "enet/enet.h"
#include "../Application/Configuration/Network/ApplicationConfiguration_Network.h"
#include "../Application/Time/Time.h"

namespace NanamiEngine::Core::Network
{
    void UnreliableSendThrottle::Tick()
    {
        const float sendInterval = 1.0f / static_cast<float>(Application::Configuration::NetworkConfiguration::GetUnreliableSendRate());
        accumulator_ += Time::DeltaTime();
        sendAllowed_ = accumulator_ >= sendInterval;
        if (sendAllowed_)
            accumulator_ = 0.0f;
    }

    void DisconnectGracefully(_ENetHost* host, _ENetPeer* peer)
    {
        if (!host || !peer || peer->state != ENET_PEER_STATE_CONNECTED)
            return;

        enet_peer_disconnect(peer, 0);

        ENetEvent event;
        const enet_uint32 start = enet_time_get();
        while (enet_time_get() - start < GRACEFUL_DISCONNECT_WAIT_MS
               && enet_host_service(host, &event, 10) >= 0)
        {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
                enet_packet_destroy(event.packet);
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                break;
        }
    }
}
