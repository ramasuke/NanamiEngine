#pragma once
#include <cmath>
#include <cstdint>

namespace Coroutine
{
    class TweenClock final
    {
    public:
        [[nodiscard]] int32_t Advance(const float seconds)
        {
            remainderMs_ += seconds * 1000.0f;
            const float wholeMs = std::floor(remainderMs_);
            remainderMs_ -= wholeMs;
            return static_cast<int32_t>(wholeMs);
        }

    private:
        float remainderMs_ = 0.0f;
    };
}
