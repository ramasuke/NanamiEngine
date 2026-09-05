#include "Ui_PlayerStatus.h"

#include "../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "../../../../../Engine/Module/GameObject/PrefabGameObject/PrefabCatchChild/PrefabCatchChild.h"
#include "../../../Core/Game/StatusParameter/Health/Health.h"
#include "../../../Core/Game/StatusParameter/Stamina/Stamina.h"

namespace GamePlay::Ui
{
    void PlayerStatus::OnAwake()
    {
        healthBar_       = GameObject::CatchChild<NanamiUi::Slider>(Entity(), healthBarName_);
        healthBarFrame_  = GameObject::CatchChild<Component::ImageRenderer>(Entity(), healthBarFrameName_);
        staminaBar_      = GameObject::CatchChild<NanamiUi::Slider>(Entity(), staminaBarName_);
        staminaBarFrame_ = GameObject::CatchChild<Component::ImageRenderer>(Entity(), staminaBarFrameName_);
        if (!injuredUiObjectName_.empty())
            injuredUiMask_ = GameObject::CatchChild<InjuredMaskUI>(Entity(), injuredUiObjectName_);
    }

    void PlayerStatus::UpdateHealthBar(
        const GameCore::StatusParameter::Health& maxHealth,
        const GameCore::StatusParameter::Health& health   ) const
    {
        healthBar_->SetValue(health / maxHealth);
    }

    void PlayerStatus::OnDamageHealthBar() const
    {
        Coroutine::StartCoroutine(OnDamagedHealth());
    }

    void PlayerStatus::UpdateStaminaBar(
        const GameCore::StatusParameter::Stamina& maxStamina,
        const GameCore::StatusParameter::Stamina& stamina   ) const
    {
        staminaBar_->SetValue(stamina / maxStamina);
    }

    Coroutine::Task<void> PlayerStatus::OnDamagedHealth() const
    {
        const auto previewSprite = healthBarFrame_->GetSprite();
        healthBarFrame_->SetSprite(onDamageHealthBarFrame_.get());
        co_await Coroutine::WaitForSeconds(displayOnDamageHealthBarDuration_secs_);
        healthBarFrame_->SetSprite(previewSprite);
    }

    void PlayerStatus::OnIsInjured(const bool isInjured) const
    {
        if (injuredUiMask_) injuredUiMask_->SetActive(isInjured);
    }

    void PlayerStatus::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("healthBarName_", healthBarName_);
        ImGuiHelper::OnDrawInputField("healthBar_", healthBar_);
        ImGuiHelper::OnDrawInputField("displayOnDamageHealthBarDuration_secs_", displayOnDamageHealthBarDuration_secs_);
        ImGuiHelper::OnDrawInputField("onDamageHealthBarFrame_", onDamageHealthBarFrame_);
        ImGuiHelper::OnDrawInputField("healthBarFrameName_", healthBarFrameName_);
        ImGuiHelper::OnDrawInputField("healthBarFrame_", healthBarFrame_);
        ImGuiHelper::OnDrawInputField("staminaBarName_", staminaBarName_);
        ImGuiHelper::OnDrawInputField("staminaBar_", staminaBar_);
        ImGuiHelper::OnDrawInputField("staminaBarFrameName_", staminaBarFrameName_);
        ImGuiHelper::OnDrawInputField("staminaBarFrame_", staminaBarFrame_);
        ImGuiHelper::OnDrawInputField("injuredUiObjectName_", injuredUiObjectName_);
        ImGuiHelper::OnDrawInputField("injuredUiMask_", injuredUiMask_);
    }
}
