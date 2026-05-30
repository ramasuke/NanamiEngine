#include "NetworkSystem.h"
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#include <enet/enet.h>

#include "../Application/Configuration/ApplicationConfiguration.h"


namespace NanamiEngine::Core::Network
{
    NetworkSystem::NetworkSystem()
    {
        enet_initialize();
        
        if constexpr (Application::Configuration::NETWORK_MODE == Mode::Server)
        {
            ENetAddress address{};
            address.host = ENET_HOST_ANY;
            address.port = 1234;

            host_ = enet_host_create(&address, 32, 2, 0, 0);
        }
        if constexpr (Application::Configuration::NETWORK_MODE == Mode::Client)
        {
            host_ = enet_host_create(nullptr, 1, 2, 0, 0);

            ENetAddress address{};
            enet_address_set_host(&address, "127.0.0.1");
            address.port = 1234;

            peer_ = enet_host_connect(host_, &address, 2, 0);
        }
    }

    NetworkSystem::~NetworkSystem()
    {
        if (host_)
            enet_host_destroy(host_);

        enet_deinitialize();
    }

    void NetworkSystem::Update()
    {
        ENetEvent event;

        while (enet_host_service(host_, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                // TODO: 接続イベント
                break;

            case ENET_EVENT_TYPE_RECEIVE:
            {
                Packet p{};
                std::memcpy(&p, event.packet->data, sizeof(Packet));

                receivedQueue_.push(p);

                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
                // TODO: 切断イベント
                break;

            default:
                break;
            }
        }
    }

    void NetworkSystem::Send(const Packet& packet)
    {
        ENetPacket* p = enet_packet_create(
            &packet,
            sizeof(Packet),
            ENET_PACKET_FLAG_RELIABLE
        );

        if constexpr (Application::Configuration::NETWORK_MODE == Mode::Server)
        {
            enet_host_broadcast(host_, 0, p);
        }
        if constexpr (Application::Configuration::NETWORK_MODE == Mode::Client)
        {
            enet_peer_send(peer_, 0, p);
        }
    }

    std::vector<Packet> NetworkSystem::PollPackets()
    {
        std::vector<Packet> result;

        while (!receivedQueue_.empty())
        {
            result.push_back(receivedQueue_.front());
            receivedQueue_.pop();
        }

        return result;
    }
}
