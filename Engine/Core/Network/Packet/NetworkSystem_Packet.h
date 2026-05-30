#pragma once
namespace NanamiEngine::Core::Network
{
    enum class PacketType
    {
        Join,
        SpawnPlayer,
        Move,
    };

    struct Packet final
    {
        PacketType type_;

        union
        {
            struct { int playerId_; } Join;
            struct { int playerId_; float x_, y_; } Spawn;
            struct { int playerId_; float x_, y_; } Move;
        };
    };
}