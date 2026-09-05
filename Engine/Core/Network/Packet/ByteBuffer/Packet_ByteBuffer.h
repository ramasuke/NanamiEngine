#pragma once
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "../cereal/include/cereal/archives/portable_binary.hpp"
#include "../../../../Module/Exception/Engine_Module_Exception.h"

namespace NanamiEngine::Core::Network
{
    class ByteBuffer final
    {
    public:
        [[nodiscard]] const uint8_t* Data() const noexcept;
        [[nodiscard]] size_t Size() const noexcept;
        [[nodiscard]] const std::vector<uint8_t>& Raw() const noexcept;

        void Append(const uint8_t* ptr, size_t size);

        template<typename T>
        void WriteRaw(const T& value)
        {
            const auto ptr = reinterpret_cast<const uint8_t*>(&value);
            Append(ptr, sizeof(T));
        }

        template<typename T>
        void Write(const T& value)
        {
            std::stringstream ss;
            cereal::PortableBinaryOutputArchive archive(ss);
            archive(value);

            const std::string str = ss.str();

            const uint32_t size = static_cast<uint32_t>(str.size());
            WriteRaw(size);

            data_.insert(data_.end(), str.begin(), str.end());
        }
        
        template<typename T>
        T ReadRaw(size_t& offset) const
        {
            // 受信データは信頼できないので assert ではなく実行時に検証する（Release でも有効）
            EnsureReadable(offset, sizeof(T));
            T value;
            memcpy(&value, data_.data() + offset, sizeof(T));
            offset += sizeof(T);
            return value;
        }

        template<typename T>
        T Read(size_t& offset) const
        {
            // サイズ取得
            const uint32_t size = ReadRaw<uint32_t>(offset);

            // バイナリ取り出し
            EnsureReadable(offset, size);
            const std::string str(reinterpret_cast<const char*>(data_.data() + offset), size);
            offset += size;

            // デシリアライズ（破損・改ざんされたペイロードは cereal が投げるので PacketDeserializeException に揃える）
            try
            {
                std::stringstream ss(str);
                cereal::PortableBinaryInputArchive archive(ss);

                T value;
                archive(value);
                return value;
            }
            catch (const std::exception& exception)
            {
                throw Module::Exception::PacketDeserializeException(exception.what());
            }
        }

    private:
        /** [offset, offset + size) がバッファ内に収まっているか検証し、超えていれば PacketDeserializeException を投げる */
        void EnsureReadable(size_t offset, size_t size) const;

        std::vector<uint8_t> data_;
    };
}
