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
        [[nodiscard]] rxcpp::observable<EnhancePower           > OnAddEnhancePowerStack() const override { return onAddEnhancePowerStack_.get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnComboAttack         () const override { return onComboAttack_         .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnDashAttack          () const override { return onDashAttack_          .get_observable(); }
        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit      > OnRun                 () const override { return onRun_                 .get_observable(); }
        
        void InvokeOnDamage   (const StatusParameter::Health& currentHealth) const override;
        void InvokeOnAddEnhancePowerStack(const EnhancePower& currentEnhancePowerStack) const override;
        void InvokeOnDeath    () const override;
        void InvokeComboAttack() const override;
        void InvokeOnRun      () const override;
        void InvokeDashAttack () const override;
        

    private:
        rxcpp::subjects::subject<StatusParameter::Health> onDamage_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onDeath_;
        rxcpp::subjects::subject<EnhancePower     >       onAddEnhancePowerStack_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onComboAttack_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onRun_;
        rxcpp::subjects::subject<LibCore::Rx::unit>       onDashAttack_;
    };
}
