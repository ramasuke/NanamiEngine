#pragma once

namespace GameCore::PlayerAvatar
{
    struct EnhancePower;
}

namespace GameCore::StatusParameter
{
    struct Health;
}

namespace GameCore::PlayerAvatar::SwordMan::State
{
    class IStatusEventSubject
    {
    public:
        virtual ~IStatusEventSubject() = default;
        
        virtual void InvokeOnDamage   (const StatusParameter::Health& currentHealth) const = 0;
        virtual void InvokeOnAddEnhancePowerStack(const EnhancePower& currentEnhancePowerStack) const = 0;
        virtual void InvokeOnDeath       () const = 0;
        virtual void InvokeComboAttack   () const = 0;
        virtual void InvokeDashAttack    () const = 0;
        virtual void InvokeOnRun         () const = 0;
        virtual void InvokeOnAvoidRolling() const = 0;
    };
}
