#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../NetworkSystem_Packet.h"
#include "../ByteBuffer/Packet_ByteBuffer.h"

namespace NanamiEngine::Core::Network
{
    // NOTE: Packet と enet に渡す／enet から受け取るバイト列との相互変換
    class NANAMI_API PacketCodec
    {
    public:
        static ByteBuffer Encode(const Packet& packet);
        static Packet Decode(const uint8_t* data, size_t size);
    };
}
