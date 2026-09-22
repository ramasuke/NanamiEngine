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
        // OnChangeHealth は購読時に現在値を流さないので、HPの数字とゲージを最初に一度そろえておく
        view.UpdateHealthBar(model.MaxHealth(), model.Health());
        model.OnChangeHealth().Subscribe([&](const GameCore::StatusParameter::Health currentHealth)
            {
                view.UpdateHealthBar(model.MaxHealth(), currentHealth);
            }).AddTo(this);
        model.Event().OnDamage().Subscribe([&view](GameCore::StatusParameter::Health) { view.OnDamageHealthBar(); }).AddTo(this);
        
        model.Stamina().Subscribe([&](const GameCore::StatusParameter::Stamina currentStamina)
            {
                view.UpdateStaminaBar(model.MaxStamina(), currentStamina);
            }).AddTo(this);

        view.OnIsInjured(model.IsInjured());
        model.OnBecomeInjured().Subscribe([&view](R4::Unit) { view.OnIsInjured(true); }).AddTo(this);
        model.OnRecoverFromInjured().Subscribe([&view](R4::Unit) { view.OnIsInjured(false); }).AddTo(this);
    }

    void StatusPresenterBase::OnDrawGui()
    {
    }
}
