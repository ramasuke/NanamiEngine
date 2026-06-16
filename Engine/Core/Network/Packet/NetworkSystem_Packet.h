#pragma once
#include <cstdint>
#include "ByteBuffer/Packet_ByteBuffer.h"

namespace NanamiEngine::Core::Network
{
    typedef std::uint8_t PacketType;

    enum class DeliveryMode : uint8_t { Reliable = 0, Unreliable = 1 };

    enum class DefaultPacketType : PacketType
    {
        AssignPlayerId     = 0,
        SpawnNetworkObject = 1,
        SyncTransform      = 2,
    };

    struct Packet final
    {
        template<typename EPacketType>
        requires(std::is_enum_v<EPacketType> || std::is_integral_v<EPacketType>)
        static Packet Create(const EPacketType eType)
        {
            const PacketType type = static_cast<PacketType>(eType);
            return Packet(type);
        }

        [[nodiscard]] ByteBuffer& Data();
        [[nodiscard]] const ByteBuffer& Data() const;
        [[nodiscard]] PacketType   Type()     const { return type_; }
        [[nodiscard]] DeliveryMode Delivery() const { return deliveryMode_; }
        void SetDelivery(DeliveryMode mode) { deliveryMode_ = mode; }

    private:
        explicit Packet(PacketType type);

        PacketType   type_         = 0;
        ByteBuffer   data_;
        DeliveryMode deliveryMode_ = DeliveryMode::Reliable;
    };
}