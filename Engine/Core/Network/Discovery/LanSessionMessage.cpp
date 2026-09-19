#include "LanSessionMessage.h"

#include "cereal/types/string.hpp"
#include "../../../Module/Exception/Engine_Module_Exception.h"

namespace NanamiEngine::Core::Network
{
    namespace
    {
        // 別のアプリや古い版のパケットを読まないための目印
        constexpr std::uint32_t LAN_SESSION_MAGIC            = 0x444C4E4E; // "NNLD"
        constexpr std::uint8_t  LAN_SESSION_PROTOCOL_VERSION = 1;

        enum class LanSessionMessageKind : std::uint8_t
        {
            Query = 1,
            Reply = 2,
        };

        ByteBuffer CreateLanSessionHeader(const LanSessionMessageKind kind)
        {
            ByteBuffer buffer;
            buffer.WriteRaw(LAN_SESSION_MAGIC);
            buffer.WriteRaw(LAN_SESSION_PROTOCOL_VERSION);
            buffer.WriteRaw(static_cast<std::uint8_t>(kind));
            return buffer;
        }

        /** ヘッダが一致すれば本体の読み出し位置を返す */
        std::optional<size_t> ReadLanSessionHeader(const ByteBuffer& buffer, const LanSessionMessageKind kind)
        {
            size_t offset = 0;
            if (buffer.ReadRaw<std::uint32_t>(offset) != LAN_SESSION_MAGIC)
                return std::nullopt;
            if (buffer.ReadRaw<std::uint8_t>(offset) != LAN_SESSION_PROTOCOL_VERSION)
                return std::nullopt;
            if (buffer.ReadRaw<std::uint8_t>(offset) != static_cast<std::uint8_t>(kind))
                return std::nullopt;
            return offset;
        }
    }

    ByteBuffer LanSessionMessage::EncodeQuery(const LanSessionQuery& query)
    {
        ByteBuffer buffer = CreateLanSessionHeader(LanSessionMessageKind::Query);
        buffer.Write(query.sessionKey);
        return buffer;
    }

    ByteBuffer LanSessionMessage::EncodeReply(const LanSessionReply& reply)
    {
        ByteBuffer buffer = CreateLanSessionHeader(LanSessionMessageKind::Reply);
        buffer.Write(reply.sessionKey);
        buffer.Write(reply.port);
        return buffer;
    }

    std::optional<LanSessionQuery> LanSessionMessage::DecodeQuery(const uint8_t* data, const size_t size)
    {
        ByteBuffer buffer;
        buffer.Append(data, size);
        try
        {
            auto offset = ReadLanSessionHeader(buffer, LanSessionMessageKind::Query);
            if (!offset)
                return std::nullopt;

            LanSessionQuery query;
            query.sessionKey = buffer.Read<std::string>(*offset);
            return query;
        }
        catch (const Module::Exception::PacketDeserializeException&)
        {
            return std::nullopt;
        }
    }

    std::optional<LanSessionReply> LanSessionMessage::DecodeReply(const uint8_t* data, const size_t size)
    {
        ByteBuffer buffer;
        buffer.Append(data, size);
        try
        {
            auto offset = ReadLanSessionHeader(buffer, LanSessionMessageKind::Reply);
            if (!offset)
                return std::nullopt;

            LanSessionReply reply;
            reply.sessionKey = buffer.Read<std::string>(*offset);
            reply.port       = buffer.Read<std::uint16_t>(*offset);
            return reply;
        }
        catch (const Module::Exception::PacketDeserializeException&)
        {
            return std::nullopt;
        }
    }
}
