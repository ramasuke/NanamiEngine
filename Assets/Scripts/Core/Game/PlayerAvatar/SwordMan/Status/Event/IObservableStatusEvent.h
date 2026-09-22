#pragma once
#include "Packages/R4/R4.h"

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
        [[nodiscard]] virtual NanamiEngine::R4::Observable<StatusParameter::Health> OnDamage              () const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnComboAttack         () const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnRun                 () const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnDashAttack          () const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnAvoidRolling        () const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnMove                () const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnJump                () const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnChargeAttack        () const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnJumpAttack          () const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnLockOn              () const = 0;
    };
}
