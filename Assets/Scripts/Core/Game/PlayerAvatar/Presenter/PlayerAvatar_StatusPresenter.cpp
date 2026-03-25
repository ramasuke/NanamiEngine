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
        const GamePlay::Ui::PlayerStatus& playerStatusView,
        const SwordManAvatarStatus& swordManStatusModel)
    {
        swordManStatusModel.Health().Subscribe(rxcpp::composite_subscription(), [&](const StatusParameter::Health currentHealth)
            {
                playerStatusView.UpdateHealthBar(swordManStatusModel.MaxHealth(), currentHealth);
            });
        swordManStatusModel.Event().OnDamage().subscribe(
                rxcpp::composite_subscription(),
                [&playerStatusView](StatusParameter::Health) { playerStatusView.OnDamageHealthBar(); },
                [](const std::exception_ptr&) { },
                [] { });
        
        swordManStatusModel.EnhancePowerStack().Subscribe(rxcpp::composite_subscription(), [&](const EnhancePower currentEnhancePowerStack)
            {
                playerStatusView.UpdateEnhancePowerStackBar(swordManStatusModel.MaxEnhancePowerStack(), currentEnhancePowerStack);
            });
        swordManStatusModel.Event().OnAddEnhancePowerStack().subscribe(
            rxcpp::composite_subscription(),
            [&playerStatusView](EnhancePower) { playerStatusView.OnAddEnhancePowerStack(); },
            [](const std::exception_ptr&) { },
            [] { });    
        swordManStatusModel.IsEnableReinforce().Subscribe(rxcpp::composite_subscription(), [&](const bool isEnableReinforce)
        {
            playerStatusView.OnIsEnableReinforceMode(isEnableReinforce);
        });
    }
}
