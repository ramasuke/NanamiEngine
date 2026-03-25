#include "Ui_PlayerStatus.h"

#include "../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../Engine/Core/Coroutine/Awaitable/WaitForSeconds/WaitForSeconds.h"
#include "../../../Core/Game/PlayerAvatar/Status/EnahancePower/EnhancePower.h"
#include "../../../Core/Game/StatusParameter/Health/Health.h"

namespace GamePlay::Ui
{
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

    void PlayerStatus::UpdateEnhancePowerStackBar(
        const GameCore::PlayerAvatar::EnhancePower& maxEnhancePower,
        const GameCore::PlayerAvatar::EnhancePower& enhancePower   ) const
    {
        enhanceBar_->SetValue(enhancePower / maxEnhancePower);
    }

    Coroutine::Task<void> PlayerStatus::OnDamagedHealth() const
    {
        const auto previewSprite = healthBarFrame_->GetSprite(); 
        healthBarFrame_->SetSprite(onDamageHealthBarFrame_.get());
        co_await Coroutine::WaitForSeconds(displayOnDamageHealthBarDuration_secs_);
        healthBarFrame_->SetSprite(previewSprite);
    }

    void PlayerStatus::OnAddEnhancePowerStack() const
    {
        Coroutine::StartCoroutine(OnAddedEnhancePowerStack());
    }

    void PlayerStatus::OnIsEnableReinforceMode(const bool enable) const
    {
        onEnableReinforceMask_->SetEnable(enable);
    }

    Coroutine::Task<void> PlayerStatus::OnAddedEnhancePowerStack() const
    {
        const auto previewSprite = enhancePowerStackBarFrame_->GetSprite(); 
        enhancePowerStackBarFrame_->SetSprite(onAddEnhancePowerStackBarFrame_.get());
        co_await Coroutine::WaitForSeconds(displayOnAddEnhancePowerStackBarDuration_secs_);
        enhancePowerStackBarFrame_->SetSprite(previewSprite);
    }

    void PlayerStatus::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("healthBar_", healthBar_);
        ImGuiHelper::OnDrawInputField("healthBarFrame_", healthBarFrame_);
        ImGuiHelper::OnDrawInputField("displayOnDamageHealthBarDuration_secs_", displayOnDamageHealthBarDuration_secs_);
        ImGuiHelper::OnDrawInputField("onDamageHealthBarFrame_", onDamageHealthBarFrame_);
        ImGuiHelper::OnDrawInputField("enhanceBar_", enhanceBar_);
        ImGuiHelper::OnDrawInputField("enhancePowerStackBarFrame_", enhancePowerStackBarFrame_);
        ImGuiHelper::OnDrawInputField("displayOnAddEnhancePowerStackBarDuration_secs_", displayOnAddEnhancePowerStackBarDuration_secs_);
        ImGuiHelper::OnDrawInputField("onAddEnhancePowerStackBarFrame_", onAddEnhancePowerStackBarFrame_);
        ImGuiHelper::OnDrawInputField("onEnableReinforceMask_", onEnableReinforceMask_);
    }
}
