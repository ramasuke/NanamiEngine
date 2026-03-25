#pragma once
#include "../rxcpp/rx.hpp"

namespace LibCore::Rx
{
    struct unit;
}

namespace GameCore::PlayerAvatar
{
    struct EnhancePower;
}

namespace GameCore::StatusParameter
{
    struct Health;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class IObservableStatusEvent
    {
    public:
        virtual ~IObservableStatusEvent() = default;
        [[nodiscard]] virtual rxcpp::observable<StatusParameter::Health> OnDamage              () const = 0;
        [[nodiscard]] virtual rxcpp::observable<EnhancePower           > OnAddEnhancePowerStack() const = 0;
        [[nodiscard]] virtual rxcpp::observable<LibCore::Rx::unit      > OnComboAttack         () const = 0;
        [[nodiscard]] virtual rxcpp::observable<LibCore::Rx::unit      > OnRun                 () const = 0;
        [[nodiscard]] virtual rxcpp::observable<LibCore::Rx::unit      > OnDashAttack          () const = 0;
    };
}
