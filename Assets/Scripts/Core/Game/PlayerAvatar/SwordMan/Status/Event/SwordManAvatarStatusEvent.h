#pragma once
#include "IObservableStatusEvent.h"
#include "../../../../../../../../Libs/LibCore/Rx/SerializableSubject/unit/unit.h"
#include "../../../Status/Event/PlayerAvatar_IStatusEvent.h"
#include "../../State/IStatusEventSubject/SwordMan_State_IStatusEventSubject.h"
#include "../rxcpp/rx.hpp"

namespace GameCore::PlayerAvatar::SwordMan
{
    class StatusEvent final : public IStatusEvent,
                              public IObservableStatusEvent,
                              public State::IStatusEventSubject
    {
        [[nodiscard]] rxcpp::observable<StatusParameter::Health> OnDamage              () const override { return onDamage_              .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnComboAttack         () const override { return onComboAttack_         .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnDashAttack          () const override { return onDashAttack_          .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnRun                 () const override { return onRun_                 .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnAvoidRolling        () const override { return onAvoidRolling_        .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnMove                () const override { return onMove_                .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnJump                () const override { return onJump_                .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnChargeAttack        () const override { return onChargeAttack_        .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnJumpAttack          () const override { return onJumpAttack_          .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnLockOn              () const override { return onLockOn_              .get_observable(); }

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
        rxcpp::subjects::subject<StatusParameter::Health> onDamage_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onDeath_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onComboAttack_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onRun_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onDashAttack_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onAvoidRolling_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onMove_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onJump_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onChargeAttack_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onJumpAttack_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onLockOn_;
    };
}
