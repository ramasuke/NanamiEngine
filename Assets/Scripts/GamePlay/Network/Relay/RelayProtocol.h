#pragma once
// NanamiRelay wire protocol, shared by the relay server and every game client.
// NOTE: NanamiEngine keeps a copy at Assets/Scripts/GamePlay/Network/Relay/RelayProtocol.h - keep both identical and bump
//       PROTOCOL_VERSION whenever the bytes on the wire change.
//
// Every peer connects with PROTOCOL_VERSION as the enet connect data, then sends one request on CHANNEL_CONTROL:
//   JoinOrHost : public matchmaking. The relay answers Hosted (you are the room's host) or Joined (you joined an open room).
//   CreateRoom : a private room. The relay picks a ROOM_CODE_LENGTH-digit code and answers RoomCreated{code}.
//   JoinRoom   : the private room with that code. Joined, or a disconnect with RoomNotFound / RoomFull / SessionMismatch.
// Private rooms are never matched by JoinOrHost.
//
// Game data (CHANNEL_RELIABLE / CHANNEL_UNRELIABLE):
//   client <-> relay : the game packet as is
//   host   -> relay  : [target slot:u8 | TARGET_ALL] + game packet
//   relay  -> host   : [source slot:u8] + game packet
// The relay never looks inside a game packet.
//
// A rejected or dropped peer is disconnected with a DisconnectReason as the enet disconnect data.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace NanamiRelay
{
    // 2: CreateRoom / RoomCreated / JoinRoom and the private room disconnect reasons
    constexpr std::uint32_t PROTOCOL_VERSION = 2;
    // The relay still accepts clients down to this version; they only ever send JoinOrHost
    constexpr std::uint32_t MIN_PROTOCOL_VERSION = 1;

    constexpr std::uint8_t CHANNEL_RELIABLE   = 0;
    constexpr std::uint8_t CHANNEL_UNRELIABLE = 1;
    constexpr std::uint8_t CHANNEL_CONTROL    = 2;
    constexpr std::size_t  CHANNEL_COUNT      = 3;

    constexpr std::uint8_t TARGET_ALL = 0xFF;
    // Slots are 0..MAX_CLIENTS_PER_ROOM-1; TARGET_ALL is never a slot
    constexpr std::uint8_t MAX_CLIENTS_PER_ROOM = 254;

    constexpr std::size_t   MAX_NAME_LENGTH  = 64;
    constexpr std::size_t   ROOM_CODE_LENGTH = 6;
    constexpr std::size_t   MAX_PACKET_SIZE  = 64 * 1024;
    constexpr std::uint32_t JOIN_TIMEOUT_MS  = 5000;

    enum class ControlType : std::uint8_t
    {
        JoinOrHost  = 1, // client -> relay: appId:str8, sessionKey:str8, maxClients:u8
        Hosted      = 2, // relay -> client
        Joined      = 3, // relay -> client
        PeerJoined  = 4, // relay -> host: slot:u8
        PeerLeft    = 5, // relay -> host: slot:u8
        CreateRoom  = 6, // client -> relay: appId:str8, sessionKey:str8, maxClients:u8
        RoomCreated = 7, // relay -> client: roomCode:str8
        JoinRoom    = 8, // client -> relay: appId:str8, sessionKey:str8, roomCode:str8
    };

    enum class DisconnectReason : std::uint32_t
    {
        None            = 0,
        VersionMismatch = 1,
        BadRequest      = 2,
        JoinTimeout     = 3,
        HostLeft        = 4,
        ServerShutdown  = 5,
        RoomNotFound    = 6, // JoinRoom: no private room has that code
        RoomFull        = 7, // JoinRoom: the room has no free slot
        SessionMismatch = 8, // JoinRoom: the room was made with another sessionKey (another stage)
    };

    struct JoinOrHostRequest
    {
        std::string  appId;
        std::string  sessionKey;
        std::uint8_t maxClients = 0; // JoinOrHost / CreateRoom
        std::string  roomCode;       // JoinRoom
    };

    struct ControlMessage
    {
        ControlType  type = ControlType::Hosted;
        std::uint8_t slot = 0;     // PeerJoined / PeerLeft only
        JoinOrHostRequest request; // JoinOrHost / CreateRoom / JoinRoom only
        std::string  roomCode;     // RoomCreated only
    };

    namespace Detail
    {
        inline void WriteString8(std::vector<std::uint8_t>& out, const std::string& value)
        {
            out.push_back(static_cast<std::uint8_t>(value.size()));
            out.insert(out.end(), value.begin(), value.end());
        }

        inline bool ReadString8(const std::uint8_t* data, const std::size_t size, std::size_t& offset, std::string& value)
        {
            if (offset >= size)
                return false;
            const std::size_t length = data[offset++];
            if (length > MAX_NAME_LENGTH || offset + length > size)
                return false;
            value.assign(reinterpret_cast<const char*>(data + offset), length);
            offset += length;
            return true;
        }

        /** JoinOrHost or CreateRoom */
        inline std::vector<std::uint8_t> EncodeHostRequest(const ControlType type, const JoinOrHostRequest& request)
        {
            std::vector<std::uint8_t> out;
            out.push_back(static_cast<std::uint8_t>(type));
            WriteString8(out, request.appId);
            WriteString8(out, request.sessionKey);
            out.push_back(request.maxClients);
            return out;
        }
    }

    inline bool IsValidName(const std::string& value)
    {
        return !value.empty() && value.size() <= MAX_NAME_LENGTH;
    }

    inline bool IsValidRoomCode(const std::string& value)
    {
        if (value.size() != ROOM_CODE_LENGTH)
            return false;
        for (const char c : value)
        {
            if (c < '0' || c > '9')
                return false;
        }
        return true;
    }

    inline std::vector<std::uint8_t> EncodeJoinOrHost(const JoinOrHostRequest& request)
    {
        return Detail::EncodeHostRequest(ControlType::JoinOrHost, request);
    }

    inline std::vector<std::uint8_t> EncodeCreateRoom(const JoinOrHostRequest& request)
    {
        return Detail::EncodeHostRequest(ControlType::CreateRoom, request);
    }

    inline std::vector<std::uint8_t> EncodeJoinRoom(const JoinOrHostRequest& request)
    {
        std::vector<std::uint8_t> out;
        out.push_back(static_cast<std::uint8_t>(ControlType::JoinRoom));
        Detail::WriteString8(out, request.appId);
        Detail::WriteString8(out, request.sessionKey);
        Detail::WriteString8(out, request.roomCode);
        return out;
    }

    inline std::vector<std::uint8_t> EncodeRoomCreated(const std::string& roomCode)
    {
        std::vector<std::uint8_t> out;
        out.push_back(static_cast<std::uint8_t>(ControlType::RoomCreated));
        Detail::WriteString8(out, roomCode);
        return out;
    }

    inline std::vector<std::uint8_t> EncodeControl(const ControlType type)
    {
        return { static_cast<std::uint8_t>(type) };
    }

    inline std::vector<std::uint8_t> EncodeSlotControl(const ControlType type, const std::uint8_t slot)
    {
        return { static_cast<std::uint8_t>(type), slot };
    }

    inline std::optional<ControlMessage> DecodeControl(const std::uint8_t* data, const std::size_t size)
    {
        if (size < 1)
            return std::nullopt;

        ControlMessage message;
        message.type = static_cast<ControlType>(data[0]);
        std::size_t offset = 1;

        switch (message.type)
        {
        case ControlType::JoinOrHost:
        case ControlType::CreateRoom:
            if (!Detail::ReadString8(data, size, offset, message.request.appId)
                || !Detail::ReadString8(data, size, offset, message.request.sessionKey)
                || offset + 1 != size)
                return std::nullopt;
            message.request.maxClients = data[offset];
            if (!IsValidName(message.request.appId) || !IsValidName(message.request.sessionKey))
                return std::nullopt;
            return message;

        case ControlType::JoinRoom:
            if (!Detail::ReadString8(data, size, offset, message.request.appId)
                || !Detail::ReadString8(data, size, offset, message.request.sessionKey)
                || !Detail::ReadString8(data, size, offset, message.request.roomCode)
                || offset != size)
                return std::nullopt;
            if (!IsValidName(message.request.appId) || !IsValidName(message.request.sessionKey)
                || !IsValidRoomCode(message.request.roomCode))
                return std::nullopt;
            return message;

        case ControlType::RoomCreated:
            if (!Detail::ReadString8(data, size, offset, message.roomCode) || offset != size
                || !IsValidRoomCode(message.roomCode))
                return std::nullopt;
            return message;

        case ControlType::Hosted:
        case ControlType::Joined:
            return size == 1 ? std::optional(message) : std::nullopt;

        case ControlType::PeerJoined:
        case ControlType::PeerLeft:
            if (size != 2)
                return std::nullopt;
            message.slot = data[1];
            return message;
        }
        return std::nullopt;
    }
}
