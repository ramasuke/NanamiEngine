#pragma once
#include "rx-observable.hpp"

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

        [[nodiscard]] virtual rxcpp::observable<StatusParameter::Health> OnDamage() const = 0;
    };
}
