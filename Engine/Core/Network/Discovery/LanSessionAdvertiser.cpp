#include "LanSessionAdvertiser.h"

#include <array>

#include "LanSessionMessage.h"
#include "../../../Module/Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Core::Network
{
    LanSessionAdvertiser::LanSessionAdvertiser(std::string sessionKey, const std::uint16_t sessionPort)
        : sessionKey_(std::move(sessionKey))
        , sessionPort_(sessionPort)
    {
        enet_initialize();

        socket_ = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
        if (socket_ == ENET_SOCKET_NULL)
        {
            Module::LogWarning("LanSessionAdvertiser: ソケットを作れないため、LAN から見つけてもらえません");
            return;
        }

        ENetAddress address{};
        address.host = ENET_HOST_ANY;
        address.port = LAN_DISCOVERY_PORT;
        if (enet_socket_bind(socket_, &address) != 0)
        {
            Module::LogWarning("LanSessionAdvertiser: 探索ポート " + std::to_string(LAN_DISCOVERY_PORT) + " を確保できないため、LAN から見つけてもらえません");
            enet_socket_destroy(socket_);
            socket_ = ENET_SOCKET_NULL;
            return;
        }
        enet_socket_set_option(socket_, ENET_SOCKOPT_NONBLOCK, 1);
    }

    LanSessionAdvertiser::~LanSessionAdvertiser()
    {
        if (socket_ != ENET_SOCKET_NULL)
            enet_socket_destroy(socket_);

        enet_deinitialize();
    }

    bool LanSessionAdvertiser::IsListening() const
    {
        return socket_ != ENET_SOCKET_NULL;
    }

    const std::string& LanSessionAdvertiser::SessionKey() const
    {
        return sessionKey_;
    }

    void LanSessionAdvertiser::Update()
    {
        if (socket_ == ENET_SOCKET_NULL)
            return;

        std::array<uint8_t, LAN_DISCOVERY_MAX_MESSAGE_SIZE> received{};
        while (true)
        {
            ENetBuffer receiveBuffer;
            receiveBuffer.data       = received.data();
            receiveBuffer.dataLength = received.size();

            ENetAddress sender{};
            const int length = enet_socket_receive(socket_, &sender, &receiveBuffer, 1);
            if (length <= 0)
                break;

            const auto query = LanSessionMessage::DecodeQuery(received.data(), static_cast<size_t>(length));
            if (!query || query->sessionKey != sessionKey_)
                continue;

            const ByteBuffer reply = LanSessionMessage::EncodeReply({ sessionKey_, sessionPort_ });
            ENetBuffer replyBuffer;
            replyBuffer.data       = const_cast<uint8_t*>(reply.Data());
            replyBuffer.dataLength = reply.Size();
            enet_socket_send(socket_, &sender, &replyBuffer, 1);
        }
    }
}
