#pragma once
#include "rx-observable.hpp"

namespace GameCore::PlayerAvatar
{
    struct EnhancePower;
}

namespace GameCore::StatusParameter
{
    struct Health;
}

namespace GameCore::PlayerAvatar
{
    class IStatusEvent
    {
    public:
        virtual ~IStatusEvent() = default;

        [[nodiscard]] virtual rxcpp::observable<StatusParameter::Health> OnDamage              () const = 0;
        [[nodiscard]] virtual rxcpp::observable<EnhancePower           > OnAddEnhancePowerStack() const = 0;
    };
}
