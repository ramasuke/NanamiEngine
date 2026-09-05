#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "../../Core/Network/Packet/NetworkSystem_Packet.h"

namespace NanamiEngine::Module::Network
{
    enum class PacketDirection
    {
        Send,
        Receive,
    };

    struct PacketLogRecord
    {
        PacketDirection direction;
        Core::Network::PacketType rawType;
        std::string typeName;
        Core::Network::DeliveryMode delivery;
        std::size_t byteSize;
        float timestamp;
    };

    // NOTE: LogPacket/PacketLogHistory/ClearPacketLogHistory はすべて内部でmutexを
    // 取っているためスレッドセーフ。ネットワークスレッドから呼んでも良い。

    /** @brief パケットの送受信を記録し、同じ内容をEngineLog（Module::Log）にも流す */
    void LogPacket(PacketDirection direction, Core::Network::PacketType rawType,
                   Core::Network::DeliveryMode delivery, std::size_t byteSize);

    /** @brief NetworkLoggerWindow等が使用するスレッドセーフなログ履歴のスナップショットを返す */
    std::vector<PacketLogRecord> PacketLogHistory();
    /** @brief 保持しているパケットログ履歴をクリアする */
    void ClearPacketLogHistory();
}
