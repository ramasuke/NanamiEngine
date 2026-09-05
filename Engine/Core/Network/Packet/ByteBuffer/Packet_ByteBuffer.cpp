#include "Packet_ByteBuffer.h"

namespace NanamiEngine::Core::Network
{
    const uint8_t* ByteBuffer::Data() const noexcept
    {
        return data_.data();
    }

    size_t ByteBuffer::Size() const noexcept
    {
        return data_.size();
    }

    const std::vector<uint8_t>& ByteBuffer::Raw() const noexcept
    {
        return data_;
    }

    void ByteBuffer::Append(const uint8_t* ptr, const size_t size)
    {
        data_.insert(data_.end(), ptr, ptr + size);
    }

    void ByteBuffer::EnsureReadable(const size_t offset, const size_t size) const
    {
        // offset + size のオーバーフローを避けるため差分で比較する
        if (offset > data_.size() || size > data_.size() - offset)
        {
            throw Module::Exception::PacketDeserializeException(
                "read out of range (offset=" + std::to_string(offset) +
                ", size=" + std::to_string(size) +
                ", buffer=" + std::to_string(data_.size()) + ")");
        }
    }
}
