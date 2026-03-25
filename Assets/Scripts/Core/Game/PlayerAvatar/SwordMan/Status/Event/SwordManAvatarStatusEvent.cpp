#include "SwordManAvatarStatusEvent.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    void StatusEvent::InvokeOnDamage(const StatusParameter::Health& currentHealth) const
    {
        onDamage_.get_subscriber().on_next(currentHealth);
    }

    void StatusEvent::InvokeOnAddEnhancePowerStack(const EnhancePower& currentEnhancePowerStack) const
    {
        onAddEnhancePowerStack_.get_subscriber().on_next(currentEnhancePowerStack);
    }

    void StatusEvent::InvokeOnDeath() const
    {
        onDeath_.get_subscriber().on_next(LibCore::Rx::unit{});
    }

    void StatusEvent::InvokeComboAttack() const
    {
        onComboAttack_.get_subscriber().on_next(LibCore::Rx::unit{});   
    }

    void StatusEvent::InvokeOnRun() const
    {
        onRun_.get_subscriber().on_next(LibCore::Rx::unit{});
    }

    void StatusEvent::InvokeDashAttack() const
    {
        onDashAttack_.get_subscriber().on_next(LibCore::Rx::unit{});
    }
}
