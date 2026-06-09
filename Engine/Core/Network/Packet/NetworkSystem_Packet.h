#pragma once
#include <cstdint>
#include "ByteBuffer/Packet_ByteBuffer.h"

namespace NanamiEngine::Core::Network
{
    typedef std::uint8_t PacketType;
    enum class DefaultPacketType : PacketType
    {
        AssignPlayerId     = 0,
        SpawnNetworkObject = 1,
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
        [[nodiscard]] PacketType Type() const { return type_; }

    private:
        explicit Packet(PacketType type);
        
        PacketType type_ = 0;
        ByteBuffer data_;
    };
}