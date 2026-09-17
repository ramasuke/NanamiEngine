#include "SwordManAvatarStatusEvent.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    void StatusEvent::InvokeOnDamage(const StatusParameter::Health& currentHealth) const
    {
        onDamage_.get_subscriber().on_next(currentHealth);
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

    void StatusEvent::InvokeOnAvoidRolling() const
    {
        onAvoidRolling_.get_subscriber().on_next(LibCore::Rx::unit{});
    }

    void StatusEvent::InvokeOnMove() const
    {
        onMove_.get_subscriber().on_next(LibCore::Rx::unit{});
    }

    void StatusEvent::InvokeOnJump() const
    {
        onJump_.get_subscriber().on_next(LibCore::Rx::unit{});
    }

    void StatusEvent::InvokeChargeAttack() const
    {
        onChargeAttack_.get_subscriber().on_next(LibCore::Rx::unit{});
    }

    void StatusEvent::InvokeJumpAttack() const
    {
        onJumpAttack_.get_subscriber().on_next(LibCore::Rx::unit{});
    }

    void StatusEvent::InvokeOnLockOn() const
    {
        onLockOn_.get_subscriber().on_next(LibCore::Rx::unit{});
    }
}
