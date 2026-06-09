#pragma once
#include "../NetworkSystem_Packet.h"
#include "../ByteBuffer/Packet_ByteBuffer.h"

namespace NanamiEngine::Core::Network
{
    //Enetに渡すことが可能になるデータとEnetからの受信データの変換
    class PacketCodec
    {
    public:
        static ByteBuffer Encode(const Packet& packet);
        static Packet Decode(const uint8_t* data, size_t size);
    };
}
