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
}
