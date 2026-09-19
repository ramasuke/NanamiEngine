#include "LanSessionFinder.h"

#include <array>
#include <cstdint>

#pragma comment(lib, "iphlpapi.lib")
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "LanSessionMessage.h"
#include "../../../Module/Log/NanamiEngine_Module_Log.h"

namespace NanamiEngine::Core::Network
{
    namespace
    {
        constexpr enet_uint32 LAN_SESSION_QUERY_INTERVAL_MS = 250;

        /**
         * 稼働中の各 IPv4 アダプタのサブネットブロードキャスト(ネットワークバイト順)。
         * Windows は 255.255.255.255 を 1 つの NIC にしか出さないことがあり、仮想 NIC がある PC で LAN 側へ届かないため
         */
        std::vector<enet_uint32> CollectLanSubnetBroadcastHosts()
        {
            std::vector<enet_uint32> hosts;

            constexpr ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
            ULONG size = 16 * 1024;
            std::vector<std::uint8_t> storage;
            ULONG result = ERROR_BUFFER_OVERFLOW;
            for (int attempt = 0; attempt < 3 && result == ERROR_BUFFER_OVERFLOW; ++attempt)
            {
                storage.resize(size);
                result = GetAdaptersAddresses(AF_INET, flags, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(storage.data()), &size);
            }
            if (result != NO_ERROR)
                return hosts;

            for (auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(storage.data()); adapter; adapter = adapter->Next)
            {
                if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
                    continue;

                for (auto* unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
                {
                    const auto* socketAddress = unicast->Address.lpSockaddr;
                    if (!socketAddress || socketAddress->sa_family != AF_INET)
                        continue;

                    const ULONG prefixLength = unicast->OnLinkPrefixLength;
                    if (prefixLength == 0 || prefixLength >= 32)
                        continue;

                    const std::uint32_t hostBitsMask = ~(0xFFFFFFFFu << (32 - prefixLength));
                    const auto address = reinterpret_cast<const sockaddr_in*>(socketAddress)->sin_addr.s_addr;
                    hosts.push_back(address | htonl(hostBitsMask));
                }
            }
            return hosts;
        }

        void AddLanQueryTarget(std::vector<ENetAddress>& targets, const enet_uint32 host)
        {
            for (const auto& target : targets)
            {
                if (target.host == host)
                    return;
            }

            ENetAddress address{};
            address.host = host;
            address.port = LAN_DISCOVERY_PORT;
            targets.push_back(address);
        }
    }

    LanSessionFinder::LanSessionFinder(std::string sessionKey)
        : sessionKey_(std::move(sessionKey))
    {
        enet_initialize();

        socket_ = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
        if (socket_ == ENET_SOCKET_NULL || enet_socket_bind(socket_, nullptr) != 0)
        {
            Module::LogWarning("LanSessionFinder: ソケットを作れないため、LAN のホストを探せません");
            if (socket_ != ENET_SOCKET_NULL)
                enet_socket_destroy(socket_);
            socket_ = ENET_SOCKET_NULL;
            return;
        }
        enet_socket_set_option(socket_, ENET_SOCKOPT_NONBLOCK, 1);
        enet_socket_set_option(socket_, ENET_SOCKOPT_BROADCAST, 1);

        AddLanQueryTarget(queryTargets_, ENET_HOST_TO_NET_32(0x7F000001)); // 127.0.0.1
        AddLanQueryTarget(queryTargets_, ENET_HOST_BROADCAST);
        for (const auto host : CollectLanSubnetBroadcastHosts())
            AddLanQueryTarget(queryTargets_, host);
    }

    LanSessionFinder::~LanSessionFinder()
    {
        if (socket_ != ENET_SOCKET_NULL)
            enet_socket_destroy(socket_);

        enet_deinitialize();
    }

    void LanSessionFinder::Update()
    {
        if (socket_ == ENET_SOCKET_NULL || found_)
            return;

        const enet_uint32 now = enet_time_get();
        if (!lastQueryTime_ || now - *lastQueryTime_ >= LAN_SESSION_QUERY_INTERVAL_MS)
        {
            SendQuery();
            lastQueryTime_ = now;
        }

        ReceiveReplies();
    }

    const std::optional<HostEndpoint>& LanSessionFinder::Found() const
    {
        return found_;
    }

    void LanSessionFinder::SendQuery() const
    {
        const ByteBuffer query = LanSessionMessage::EncodeQuery({ sessionKey_ });
        ENetBuffer buffer;
        buffer.data       = const_cast<uint8_t*>(query.Data());
        buffer.dataLength = query.Size();

        for (const auto& target : queryTargets_)
            enet_socket_send(socket_, &target, &buffer, 1);
    }

    void LanSessionFinder::ReceiveReplies()
    {
        std::array<uint8_t, LAN_DISCOVERY_MAX_MESSAGE_SIZE> received{};
        while (!found_)
        {
            ENetBuffer buffer;
            buffer.data       = received.data();
            buffer.dataLength = received.size();

            ENetAddress sender{};
            const int length = enet_socket_receive(socket_, &sender, &buffer, 1);
            if (length <= 0)
                break;

            const auto reply = LanSessionMessage::DecodeReply(received.data(), static_cast<size_t>(length));
            if (!reply || reply->sessionKey != sessionKey_)
                continue;

            std::array<char, 64> hostIp{};
            if (enet_address_get_host_ip(&sender, hostIp.data(), hostIp.size()) != 0)
                continue;

            found_ = HostEndpoint{ hostIp.data(), reply->port };
        }
    }
}
