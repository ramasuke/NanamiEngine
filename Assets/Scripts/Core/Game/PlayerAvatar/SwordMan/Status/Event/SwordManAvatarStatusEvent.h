#pragma once
#include "IObservableStatusEvent.h"
#include "Packages/R4/R4.h"
#include "../../../Status/Event/PlayerAvatar_IStatusEvent.h"
#include "../../State/IStatusEventSubject/SwordMan_State_IStatusEventSubject.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    class StatusEvent final : public IStatusEvent,
                              public IObservableStatusEvent,
                              public State::IStatusEventSubject
    {
        [[nodiscard]] NanamiEngine::R4::Observable<StatusParameter::Health> OnDamage              () const override { return onDamage_              .AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnComboAttack         () const override { return onComboAttack_         .AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnDashAttack          () const override { return onDashAttack_          .AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnRun                 () const override { return onRun_                 .AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnAvoidRolling        () const override { return onAvoidRolling_        .AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnMove                () const override { return onMove_                .AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnJump                () const override { return onJump_                .AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnChargeAttack        () const override { return onChargeAttack_        .AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnJumpAttack          () const override { return onJumpAttack_          .AsObservable(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit      > OnLockOn              () const override { return onLockOn_              .AsObservable(); }

        void InvokeOnDamage   (const StatusParameter::Health& currentHealth) const override;
        void InvokeOnDeath       () const override;
        void InvokeComboAttack   () const override;
        void InvokeOnRun         () const override;
        void InvokeDashAttack    () const override;
        void InvokeOnAvoidRolling() const override;
        void InvokeOnMove        () const override;
        void InvokeOnJump        () const override;
        void InvokeChargeAttack  () const override;
        void InvokeJumpAttack    () const override;
        void InvokeOnLockOn      () const override;

    private:
        NanamiEngine::R4::Subject<StatusParameter::Health> onDamage_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>       onDeath_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>       onComboAttack_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>       onRun_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>       onDashAttack_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>       onAvoidRolling_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>       onMove_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>       onJump_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>       onChargeAttack_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>       onJumpAttack_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit>       onLockOn_;
    };
}
