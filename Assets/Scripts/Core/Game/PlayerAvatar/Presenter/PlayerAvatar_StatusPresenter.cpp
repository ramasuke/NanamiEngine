#include "PlayerAvatar_StatusPresenter.h"

#include "../../../../GamePlay/Ui/PlayerStatus/Ui_PlayerStatus.h"
#include "../rxcpp/rx.hpp"
#include "../SwordMan/Status/SwordManAvatarStatus.h"
#include "../SwordMan/Status/Event/SwordManAvatarStatusEvent.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    StatusPresenter::StatusPresenter(
        const GamePlay::Ui::PlayerStatus& playerStatusView,
        const SwordManAvatarStatus& playerStatusModel)
    {
        SubscribeModelEventToView(playerStatusView, playerStatusModel);
    }

    void StatusPresenter::SubscribeModelEventToView(
        const GamePlay::Ui::PlayerStatus& view,
        const SwordManAvatarStatus& model)
    {
        auto onDestroySubscription = rxcpp::composite_subscription();
        
        model.Health().Subscribe(onDestroySubscription, [&](const StatusParameter::Health currentHealth)
            {
                view.UpdateHealthBar(model.MaxHealth(), currentHealth);
            });
        model.Event().OnDamage().subscribe(
                onDestroySubscription,
                [&view](StatusParameter::Health) { view.OnDamageHealthBar(); });
        
        model.EnhancePowerStack().Subscribe(onDestroySubscription, [&](const EnhancePower currentEnhancePowerStack)
            {
                view.UpdateEnhancePowerStackBar(model.MaxEnhancePowerStack(), currentEnhancePowerStack);
            });
        model.Event().OnAddEnhancePowerStack().subscribe(
            onDestroySubscription,
            [&view](EnhancePower) { view.OnAddEnhancePowerStack(); });    
        model.IsEnableReinforce().Subscribe(onDestroySubscription, [&](const bool isEnableReinforce)
        {
            view.OnIsEnableReinforceMode(isEnableReinforce);
        });
    }
}
