#include "SwordManAvatarStatusEvent.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    void StatusEvent::InvokeOnDamage(const StatusParameter::Health& currentHealth) const
    {
        onDamage_.OnNext(currentHealth);
    }

    void StatusEvent::InvokeOnDeath() const
    {
        onDeath_.OnNext(NanamiEngine::R4::Unit{});
    }

    void StatusEvent::InvokeComboAttack() const
    {
        onComboAttack_.OnNext(NanamiEngine::R4::Unit{});   
    }

    void StatusEvent::InvokeOnRun() const
    {
        onRun_.OnNext(NanamiEngine::R4::Unit{});
    }

    void StatusEvent::InvokeDashAttack() const
    {
        onDashAttack_.OnNext(NanamiEngine::R4::Unit{});
    }

    void StatusEvent::InvokeOnAvoidRolling() const
    {
        onAvoidRolling_.OnNext(NanamiEngine::R4::Unit{});
    }

    void StatusEvent::InvokeOnMove() const
    {
        onMove_.OnNext(NanamiEngine::R4::Unit{});
    }

    void StatusEvent::InvokeOnJump() const
    {
        onJump_.OnNext(NanamiEngine::R4::Unit{});
    }

    void StatusEvent::InvokeChargeAttack() const
    {
        onChargeAttack_.OnNext(NanamiEngine::R4::Unit{});
    }

    void StatusEvent::InvokeJumpAttack() const
    {
        onJumpAttack_.OnNext(NanamiEngine::R4::Unit{});
    }

    void StatusEvent::InvokeOnLockOn() const
    {
        onLockOn_.OnNext(NanamiEngine::R4::Unit{});
    }
}
