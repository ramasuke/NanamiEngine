#pragma once
#include <cstdint>

namespace NanamiEngine::Core::Network
{
    enum class DeliveryMode : uint8_t { Reliable = 0, Unreliable = 1 };
}
