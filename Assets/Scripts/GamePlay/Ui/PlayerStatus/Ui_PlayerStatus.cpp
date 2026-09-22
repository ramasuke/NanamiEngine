#include "Ui_PlayerStatus.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "../../../Core/Game/StatusParameter/Health/Health.h"
#include "../../../Core/Game/StatusParameter/Stamina/Stamina.h"

namespace GamePlay::Ui
{
    void PlayerStatus::UpdateHealthBar(
        const GameCore::StatusParameter::Health& maxHealth,
        const GameCore::StatusParameter::Health& health   ) const
    {
        const float healthRate = health / maxHealth;
        healthBar_->SetValue(healthRate);
        if (const auto gaugeSprite = SelectHealthGaugeSprite(healthRate))
            healthBar_->ChangeGaugeSprite(gaugeSprite);
        healthBar_->SetPulse(healthGaugeDangerSprite_ && healthRate > 0.0f && healthRate <= dangerHealthRate_);

        if (hpCurrentText_)
        {
            hpCurrentText_->SetText(std::to_string(health.Value()));
            hpCurrentText_->SetTextColor(SelectHealthTextColor(healthRate));
        }
        if (hpMaxText_)
            hpMaxText_->SetText("/" + std::to_string(maxHealth.Value()));
    }

    Color32 PlayerStatus::SelectHealthTextColor(const float healthRate) const
    {
        if (damageFlashCount_ > 0 || healthRate <= dangerHealthRate_)
            return healthTextDangerColor_;
        if (healthRate <= cautionHealthRate_)
            return healthTextCautionColor_;
        return healthTextNormalColor_;
    }

    std::shared_ptr<Asset::SpriteFile> PlayerStatus::SelectHealthGaugeSprite(const float healthRate) const
    {
        if (healthRate <= dangerHealthRate_ && healthGaugeDangerSprite_)
            return healthGaugeDangerSprite_.get();
        if (healthRate <= cautionHealthRate_ && healthGaugeCautionSprite_)
            return healthGaugeCautionSprite_.get();
        if (healthGaugeNormalSprite_)
            return healthGaugeNormalSprite_.get();
        return nullptr;
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
        ++damageFlashCount_;
        if (hpCurrentText_)
            hpCurrentText_->SetTextColor(healthTextDangerColor_);

        const bool isSwapFrame = static_cast<bool>(onDamageHealthBarFrame_);
        const auto previewSprite = isSwapFrame ? healthBarFrame_->GetSprite() : std::weak_ptr<Asset::SpriteFile>();
        if (isSwapFrame)
            healthBarFrame_->SetSprite(onDamageHealthBarFrame_.get());

        co_await Coroutine::WaitForSeconds(displayOnDamageHealthBarDuration_secs_);

        if (isSwapFrame)
            healthBarFrame_->SetSprite(previewSprite);
        --damageFlashCount_;
        if (hpCurrentText_)
            hpCurrentText_->SetTextColor(SelectHealthTextColor(healthBar_->GetValue()));
    }

    void PlayerStatus::OnIsInjured(const bool isInjured) const
    {
        if (injuredUiMask_) injuredUiMask_->SetActive(isInjured);
    }

    void PlayerStatus::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("healthBar_", healthBar_);
        ImGuiHelper::OnDrawInputField("displayOnDamageHealthBarDuration_secs_", displayOnDamageHealthBarDuration_secs_);
        ImGuiHelper::OnDrawInputField("onDamageHealthBarFrame_", onDamageHealthBarFrame_);
        ImGuiHelper::OnDrawInputField("healthBarFrame_", healthBarFrame_);
        ImGuiHelper::OnDrawInputField("staminaBar_", staminaBar_);
        ImGuiHelper::OnDrawInputField("staminaBarFrame_", staminaBarFrame_);
        ImGuiHelper::OnDrawInputField("injuredUiMask_", injuredUiMask_);
        ImGuiHelper::OnDrawInputField("hpCurrentText_", hpCurrentText_);
        ImGuiHelper::OnDrawInputField("hpMaxText_", hpMaxText_);
        ImGuiHelper::OnDrawInputField("healthGaugeNormalSprite_", healthGaugeNormalSprite_);
        ImGuiHelper::OnDrawInputField("healthGaugeCautionSprite_", healthGaugeCautionSprite_);
        ImGuiHelper::OnDrawInputField("healthGaugeDangerSprite_", healthGaugeDangerSprite_);
        ImGuiHelper::OnDrawInputField("cautionHealthRate_", cautionHealthRate_);
        ImGuiHelper::OnDrawInputField("dangerHealthRate_", dangerHealthRate_);
        ImGuiHelper::OnDrawInputField("healthTextNormalColor_", healthTextNormalColor_);
        ImGuiHelper::OnDrawInputField("healthTextCautionColor_", healthTextCautionColor_);
        ImGuiHelper::OnDrawInputField("healthTextDangerColor_", healthTextDangerColor_);
    }
}
