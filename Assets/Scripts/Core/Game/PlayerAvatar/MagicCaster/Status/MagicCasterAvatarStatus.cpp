#include "MagicCasterAvatarStatus.h"

#include <algorithm>
#include <cassert>

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../Damage/Game_Damage_IDamage.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    MagicCasterAvatarStatus::MagicCasterAvatarStatus()
        : maxHealth_(80)
        , maxStamina_(StatusParameter::Stamina(100.0f))
        , stamina_(StatusParameter::Stamina(100.0f))
        , staminaDrainPerSecond_(10.0f)
        , staminaRegenPerSecond_(30.0f)
        , minStaminaRatioToResumeRun_(0.3f)
        , walkSpeed_(20.0f)
        , runSpeed_(55.0f)
        , moveRotateSpeed_(6.2f)
        , jumpPower_(65.0f)
        , jumpStateDuration_secs_(0.45f)
        , jumpCooldown_secs_(0.5f)
        , jumpStaminaCost_(15.0f)
        , castDamage_(Damage::PhysicsPower(8))
        , castStaminaCost_(20.0f)
        , castCooldown_secs_(0.3f)
        , damageStateDuration_secs_(1.0f)
        , deathStateDuration_secs_(1.8f)
    {
    }

    MagicCasterAvatarStatus::~MagicCasterAvatarStatus() = default;

    void MagicCasterAvatarStatus::Init()
    {
    }

    void MagicCasterAvatarStatus::OnUpdate()
    {
        if (jumpCooldownRemaining_secs_ > 0.0f)
        {
            jumpCooldownRemaining_secs_ -= Time::DeltaTime();
            jumpCooldownRemaining_secs_ = (std::max)(jumpCooldownRemaining_secs_, 0.0f);
        }
        if (castCooldownRemaining_secs_ > 0.0f)
        {
            castCooldownRemaining_secs_ -= Time::DeltaTime();
            castCooldownRemaining_secs_ = (std::max)(castCooldownRemaining_secs_, 0.0f);
        }

        assert(stateMachine_ && "MagicCasterAvatarStatus: stateMachine_ is not set");
        switch (stateMachine_->GetCurrentStateType())
        {
        case MagicCasterAvatarStateType::Run:
        {
            const auto drained = stamina_.get() - StatusParameter::Stamina(staminaDrainPerSecond_ * Time::DeltaTime());
            if (drained <= StatusParameter::Stamina(0.0f))
            {
                stamina_.OnNext(StatusParameter::Stamina(0.0f));
                isStaminaExhausted_ = true;
            }
            else
            {
                stamina_.OnNext(drained);
            }
            break;
        }
        case MagicCasterAvatarStateType::Jump:
        case MagicCasterAvatarStateType::Floating:
            break;
        default:
        {
            const auto regened = stamina_.get() + StatusParameter::Stamina(staminaRegenPerSecond_ * Time::DeltaTime());
            if (maxStamina_ <= regened)
            {
                stamina_.OnNext(maxStamina_);
            }
            else
            {
                stamina_.OnNext(regened);
            }
            if (isStaminaExhausted_ && stamina_.get() >= StatusParameter::Stamina(maxStamina_.Value() * minStaminaRatioToResumeRun_))
            {
                isStaminaExhausted_ = false;
            }
            break;
        }
        }
    }

    bool MagicCasterAvatarStatus::IsDamaged() const
    {
        return !onDamagedStack_.empty();
    }

    void MagicCasterAvatarStatus::AddOnDamageStack(std::unique_ptr<IDamage> damageContext)
    {
        onDamagedStack_.push(std::move(damageContext));
    }

    void MagicCasterAvatarStatus::ApplyDamage()
    {
        while (!onDamagedStack_.empty())
        {
            const auto damageContext = std::move(onDamagedStack_.front());
            onDamagedStack_.pop();
            currentHealth_->Set(StatusParameter::Health(currentHealth_->Get().Value() - damageContext->DamageValue()));
            onChangeHealth_.get_subscriber().on_next(currentHealth_->Get());
            event_->onDamage_.get_subscriber().on_next(currentHealth_->Get());
        }
    }

    void MagicCasterAvatarStatus::DiscardDamage()
    {
        std::queue<std::unique_ptr<IDamage>> empty;
        std::swap(onDamagedStack_, empty);
    }

    void MagicCasterAvatarStatus::ConsumeStamina(const float cost)
    {
        const auto consumed = stamina_.get() - StatusParameter::Stamina(cost);
        if (consumed <= StatusParameter::Stamina(0.0f))
        {
            stamina_.OnNext(StatusParameter::Stamina(0.0f));
            isStaminaExhausted_ = true;
        }
        else
        {
            stamina_.OnNext(consumed);
        }
    }

    void MagicCasterAvatarStatus::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("maxHealth_", maxHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("health_", currentHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("maxStamina_", maxStamina_);
        LibCore::ImGuiHelper::OnDrawInputField("stamina_", stamina_);
        LibCore::ImGuiHelper::OnDrawInputField("staminaDrainPerSecond_", staminaDrainPerSecond_);
        LibCore::ImGuiHelper::OnDrawInputField("staminaRegenPerSecond_", staminaRegenPerSecond_);
        LibCore::ImGuiHelper::OnDrawInputField("minStaminaRatioToResumeRun_", minStaminaRatioToResumeRun_);
        LibCore::ImGuiHelper::OnDrawInputField("walkSpeed_", walkSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("runSpeed_", runSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("moveRotateSpeed_", moveRotateSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpPower_", jumpPower_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpStateDuration_secs_", jumpStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpCooldown_secs_", jumpCooldown_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpStaminaCost_", jumpStaminaCost_);
        LibCore::ImGuiHelper::OnDrawInputField("castDamage_", castDamage_);
        LibCore::ImGuiHelper::OnDrawInputField("castStaminaCost_", castStaminaCost_);
        LibCore::ImGuiHelper::OnDrawInputField("castCooldown_secs_", castCooldown_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("damageStateDuration_secs_", damageStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("deathStateDuration_secs_", deathStateDuration_secs_);
    }
}
