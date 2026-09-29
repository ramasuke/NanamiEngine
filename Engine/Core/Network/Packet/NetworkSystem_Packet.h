#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "ByteBuffer/Packet_ByteBuffer.h"
#include "NetworkSystem_PacketType.h"
#include "NetworkSystem_DeliveryMode.h"
#include "NetworkSystem_DefaultPacketType.h"

namespace NanamiEngine::Core::Network
{
    struct NANAMI_API Packet final
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