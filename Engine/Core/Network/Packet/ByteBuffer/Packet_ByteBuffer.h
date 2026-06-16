#pragma once
#include <cassert>
#include <iosfwd>
#include <vector>

#include "../cereal/include/cereal/archives/portable_binary.hpp"

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
            assert(offset + sizeof(T) <= data_.size());
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
            assert(offset + size <= data_.size());
            const std::string str(reinterpret_cast<const char*>(data_.data() + offset), size);
            offset += size;

            // デシリアライズ
            std::stringstream ss(str);
            cereal::PortableBinaryInputArchive archive(ss);

            T value;
            archive(value);
            return value;
        }
        
    private:
        std::vector<uint8_t> data_;
    };
}
