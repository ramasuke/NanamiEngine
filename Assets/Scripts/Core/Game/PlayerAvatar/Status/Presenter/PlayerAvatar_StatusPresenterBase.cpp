#include "PlayerAvatar_StatusPresenterBase.h"

#include "../IPlayerAvatarStatus.h"
#include "../../../../../GamePlay/Ui/PlayerStatus/Ui_PlayerStatus.h"
#include "../Event/PlayerAvatar_IStatusEvent.h"

namespace GamePlay::PlayerAvatar
{
    void StatusPresenterBase::Initialize(
        const Ui::PlayerStatus& playerStatusView,
        const GameCore::PlayerAvatar::IPlayerAvatarStatus& playerStatusModel)
    {
        SubscribeModelEventToView(playerStatusView, playerStatusModel);
    }

    void StatusPresenterBase::SubscribeModelEventToView(
        const Ui::PlayerStatus& view,
        const GameCore::PlayerAvatar::IPlayerAvatarStatus& model)
    {
        auto onDestroySubscription = rxcpp::composite_subscription();
        
        model.OnChangeHealth().subscribe(onDestroySubscription, [&](const GameCore::StatusParameter::Health currentHealth)
            {
                view.UpdateHealthBar(model.MaxHealth(), currentHealth);
            });
        model.Event().OnDamage().subscribe(
                onDestroySubscription,
                [&view](GameCore::StatusParameter::Health) { view.OnDamageHealthBar(); });
        
        model.EnhancePowerStack().Subscribe(onDestroySubscription, [&](const GameCore::PlayerAvatar::EnhancePower currentEnhancePowerStack)
            {
                view.UpdateEnhancePowerStackBar(model.MaxEnhancePowerStack(), currentEnhancePowerStack);
            });
        model.Event().OnAddEnhancePowerStack().subscribe(
            onDestroySubscription,
            [&view](GameCore::PlayerAvatar::EnhancePower) { view.OnAddEnhancePowerStack(); });    
        model.IsEnableReinforce().Subscribe(onDestroySubscription, [&](const bool isEnableReinforce)
        {
            view.OnIsEnableReinforceMode(isEnableReinforce);
        });
    }

    void StatusPresenterBase::OnDrawGui()
    {
    }
}
