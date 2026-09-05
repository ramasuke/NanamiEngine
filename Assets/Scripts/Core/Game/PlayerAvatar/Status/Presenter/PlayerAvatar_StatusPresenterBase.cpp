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
        
        model.Stamina().Subscribe(onDestroySubscription, [&](const GameCore::StatusParameter::Stamina currentStamina)
            {
                view.UpdateStaminaBar(model.MaxStamina(), currentStamina);
            });

        view.OnIsInjured(model.IsInjured());
        model.OnBecomeInjured().subscribe(onDestroySubscription, [&view](LibCore::Rx::unit) { view.OnIsInjured(true); });
        model.OnRecoverFromInjured().subscribe(onDestroySubscription, [&view](LibCore::Rx::unit) { view.OnIsInjured(false); });
    }

    void StatusPresenterBase::OnDrawGui()
    {
    }
}
