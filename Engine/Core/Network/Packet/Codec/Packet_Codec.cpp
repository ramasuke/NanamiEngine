#include "Packet_Codec.h"

namespace NanamiEngine::Core::Network
{
    ByteBuffer PacketCodec::Encode(const Packet& packet)
    {
        ByteBuffer buffer;
        buffer.WriteRaw(packet.Type());
        buffer.WriteRaw(static_cast<uint8_t>(packet.Delivery()));
        const uint32_t size = static_cast<uint32_t>(packet.Data().Size());
        buffer.WriteRaw(size);

        buffer.Append(
            packet.Data().Data(),
            packet.Data().Size()
        );

        return buffer;
    }

    Packet PacketCodec::Decode(const uint8_t* data, const size_t size)
    {
        ByteBuffer buffer;
        buffer.Append(data, size);

        size_t offset = 0;

        const PacketType   type         = buffer.ReadRaw<PacketType>(offset);
        const DeliveryMode deliveryMode = static_cast<DeliveryMode>(buffer.ReadRaw<uint8_t>(offset));
        const uint32_t     payloadSize  = buffer.ReadRaw<uint32_t>(offset);

        Packet p = Packet::Create(type);
        p.Data().Append(
            buffer.Data() + offset,
            payloadSize
        );
        p.SetDelivery(deliveryMode);

        return p;
    }
}
