#include "Engine_Network_PacketLog.h"

#include <deque>
#include <mutex>

#include "Engine_Network_PacketTypeNameRegistry.h"
#include "../Log/NanamiEngine_Module_Log.h"
#include "../../Core/Application/Time/Time.h"

namespace NanamiEngine::Module::Network
{
    namespace
    {
        // NetworkLoggerWindow等に表示するログ履歴の上限件数（超えた分は古いものから捨てる）
        constexpr size_t kMaxPacketLogHistory = 2000;

        std::mutex& PacketLogMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        std::deque<PacketLogRecord>& PacketLogHistoryBuffer()
        {
            static std::deque<PacketLogRecord> history;
            return history;
        }
    }

    void LogPacket(const PacketDirection direction, const Core::Network::PacketType rawType,
                   const Core::Network::DeliveryMode delivery, const std::size_t byteSize)
    {
        const std::string typeName = PacketTypeNameRegistry::Instance().Resolve(rawType);

        {
            std::lock_guard lock(PacketLogMutex());
            auto& history = PacketLogHistoryBuffer();
            history.push_back(PacketLogRecord{ direction, rawType, typeName, delivery, byteSize, Time::CurrentTime() });
            if (history.size() > kMaxPacketLogHistory)
                history.pop_front();
        }

        const std::string directionText = direction == PacketDirection::Send ? "Send" : "Recv";
        const std::string deliveryText  = delivery == Core::Network::DeliveryMode::Reliable ? "Reliable" : "Unreliable";
        Log("[Network][" + directionText + "] " + typeName + " " + std::to_string(byteSize) + "B " + deliveryText);
    }

    std::vector<PacketLogRecord> PacketLogHistory()
    {
        std::lock_guard lock(PacketLogMutex());
        const auto& history = PacketLogHistoryBuffer();
        return std::vector<PacketLogRecord>(history.begin(), history.end());
    }

    void ClearPacketLogHistory()
    {
        std::lock_guard lock(PacketLogMutex());
        PacketLogHistoryBuffer().clear();
    }
}
