#pragma once
// NanamiRelay の通信プロトコル。中継サーバーとすべてのゲームクライアントで共有する。
// 注意: NanamiEngine 側にも Assets/Scripts/GamePlay/Network/Relay/RelayProtocol.h として同じものがある。両方をそろえ、
//       やり取りするバイト列が変わるときは PROTOCOL_VERSION を上げること。
//
// どの peer も enet の connect data に PROTOCOL_VERSION を入れて接続し、CHANNEL_CONTROL でリクエストを 1 つ送る:
//   JoinOrHost : 公開マッチング。中継は Hosted（部屋のホストになった）か Joined（空いている部屋に入った）を返す。
//   CreateRoom : 非公開部屋。中継が ROOM_CODE_LENGTH 桁のコードを選び、RoomCreated{code} を返す。
//   JoinRoom   : そのコードの非公開部屋に入る。Joined か、RoomNotFound / RoomFull / SessionMismatch で切断。
// 非公開部屋が JoinOrHost で選ばれることはない。
//
// ゲームデータ（CHANNEL_RELIABLE / CHANNEL_UNRELIABLE）:
//   クライアント <-> 中継 : ゲームのパケットそのまま
//   ホスト       -> 中継  : [宛先 slot:u8 | TARGET_ALL] + ゲームのパケット
//   中継         -> ホスト: [送り主 slot:u8] + ゲームのパケット
// 中継はゲームのパケットの中身を一切見ない。
//
// 拒否・切断する peer には、enet の disconnect data に DisconnectReason を入れて切断する。

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace NanamiRelay
{
    // 2: CreateRoom / RoomCreated / JoinRoom と、非公開部屋の切断理由を追加
    constexpr std::uint32_t PROTOCOL_VERSION = 2;
    // 中継はこのバージョンまでのクライアントを受け付ける（古いクライアントは JoinOrHost しか送らない）
    constexpr std::uint32_t MIN_PROTOCOL_VERSION = 1;

    constexpr std::uint8_t CHANNEL_RELIABLE   = 0;
    constexpr std::uint8_t CHANNEL_UNRELIABLE = 1;
    constexpr std::uint8_t CHANNEL_CONTROL    = 2;
    constexpr std::size_t  CHANNEL_COUNT      = 3;

    constexpr std::uint8_t TARGET_ALL = 0xFF;
    // slot は 0..MAX_CLIENTS_PER_ROOM-1。TARGET_ALL が slot になることはない
    constexpr std::uint8_t MAX_CLIENTS_PER_ROOM = 254;

    constexpr std::size_t   MAX_NAME_LENGTH  = 64;
    constexpr std::size_t   ROOM_CODE_LENGTH = 6;
    constexpr std::size_t   MAX_PACKET_SIZE  = 64 * 1024;
    constexpr std::uint32_t JOIN_TIMEOUT_MS  = 5000;

    enum class ControlType : std::uint8_t
    {
        JoinOrHost  = 1, // クライアント -> 中継: appId:str8, sessionKey:str8, maxClients:u8
        Hosted      = 2, // 中継 -> クライアント
        Joined      = 3, // 中継 -> クライアント
        PeerJoined  = 4, // 中継 -> ホスト: slot:u8
        PeerLeft    = 5, // 中継 -> ホスト: slot:u8
        CreateRoom  = 6, // クライアント -> 中継: appId:str8, sessionKey:str8, maxClients:u8
        RoomCreated = 7, // 中継 -> クライアント: roomCode:str8
        JoinRoom    = 8, // クライアント -> 中継: appId:str8, sessionKey:str8, roomCode:str8
    };

    enum class DisconnectReason : std::uint32_t
    {
        None            = 0,
        VersionMismatch = 1,
        BadRequest      = 2,
        JoinTimeout     = 3,
        HostLeft        = 4,
        ServerShutdown  = 5,
        RoomNotFound    = 6, // JoinRoom: そのコードの非公開部屋が無い
        RoomFull        = 7, // JoinRoom: 部屋に空きが無い
        SessionMismatch = 8, // JoinRoom: 部屋が別の sessionKey（別のステージ）で作られている
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
        std::uint8_t slot = 0;     // PeerJoined / PeerLeft のときだけ
        JoinOrHostRequest request; // JoinOrHost / CreateRoom / JoinRoom のときだけ
        std::string  roomCode;     // RoomCreated のときだけ
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

        /** JoinOrHost か CreateRoom */
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
