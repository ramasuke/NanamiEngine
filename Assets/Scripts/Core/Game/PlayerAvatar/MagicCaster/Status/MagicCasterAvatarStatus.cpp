#include "MagicCasterAvatarStatus.h"

#include <algorithm>
#include <cassert>

#include "Engine/Core/Application/Time/Time.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../Damage/Game_Damage_IDamage.h"
#include "../../../Magic/IMagicSpell.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    MagicCasterAvatarStatus::MagicCasterAvatarStatus()
        : maxHealth_(80)
        , currentHealth_(SyncParamFactory::Create<StatusParameter::Health>(this, maxHealth_))
        , maxStamina_(StatusParameter::Stamina(100.0f))
        , stamina_(StatusParameter::Stamina(100.0f))
        , staminaDrainPerSecond_(10.0f)
        , staminaRegenPerSecond_(30.0f)
        , minStaminaRatioToResumeRun_(0.3f)
        , walkSpeed_(20.0f)
        , runSpeed_(55.0f)
        , moveRotateSpeed_(4.65f)
        , jumpPower_(65.0f)
        , jumpStateDuration_secs_(0.45f)
        , jumpCooldown_secs_(0.5f)
        , jumpStaminaCost_(15.0f)
        , avoidRollingStateDuration_secs_(0.4090909064f)
        , avoidRollingStaminaCost_(20.0f)
        , damageStateDuration_secs_(1.0f)
        , deathStateDuration_secs_(1.8f)
        , maxMana_(StatusParameter::Mana(100.0f))
        , mana_(StatusParameter::Mana(100.0f))
        , manaRegenPerSecond_(4.0f)
        , wallet_(std::make_shared<PlayerAvatar::Wallet>())
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
        for (auto& remaining : cooldownRemaining_secs_)
            remaining = (std::max)(remaining - Time::DeltaTime(), 0.0f);

        if (attackBuffRemaining_secs_ > 0.0f)
            attackBuffRemaining_secs_ = (std::max)(attackBuffRemaining_secs_ - Time::DeltaTime(), 0.0f);

        if (invincibleRemaining_secs_ > 0.0f)
            invincibleRemaining_secs_ = (std::max)(invincibleRemaining_secs_ - Time::DeltaTime(), 0.0f);

        if (!IsDeath() && mana_.Value() < maxMana_)
        {
            const auto regened = mana_.Value() + StatusParameter::Mana(manaRegenPerSecond_ * Time::DeltaTime());
            mana_.Value(maxMana_ <= regened ? maxMana_ : regened);
        }

        assert(stateMachine_ && "MagicCasterAvatarStatus: stateMachine_ is not set");
        switch (stateMachine_->GetCurrentStateType())
        {
        case MagicCasterAvatarStateType::Run:
        {
            const auto drained = stamina_.Value() - StatusParameter::Stamina(staminaDrainPerSecond_ * Time::DeltaTime());
            if (drained <= StatusParameter::Stamina(0.0f))
            {
                stamina_.Value(StatusParameter::Stamina(0.0f));
                isStaminaExhausted_ = true;
            }
            else
            {
                stamina_.Value(drained);
            }
            break;
        }
        case MagicCasterAvatarStateType::AvoidRolling:
        case MagicCasterAvatarStateType::Jump:
        case MagicCasterAvatarStateType::Floating:
            break;
        default:
        {
            const auto regened = stamina_.Value() + StatusParameter::Stamina(staminaRegenPerSecond_ * Time::DeltaTime());
            if (maxStamina_ <= regened)
            {
                stamina_.Value(maxStamina_);
            }
            else
            {
                stamina_.Value(regened);
            }
            if (isStaminaExhausted_ && stamina_.Value() >= StatusParameter::Stamina(maxStamina_.Value() * minStaminaRatioToResumeRun_))
            {
                isStaminaExhausted_ = false;
            }
            break;
        }
        }

        const bool currentlyInjured = IsInjured();
        if (currentlyInjured && !wasInjured_)
            onBecomeInjured_.OnNext(NanamiEngine::R4::Unit{});
        else if (!currentlyInjured && wasInjured_)
            onRecoverFromInjured_.OnNext(NanamiEngine::R4::Unit{});
        wasInjured_ = currentlyInjured;
    }

    bool MagicCasterAvatarStatus::IsInjured() const
    {
        const auto maxVal = static_cast<float>(maxHealth_.Value());
        if (maxVal <= 0.0f) return false;
        return static_cast<float>(currentHealth_->Get().Value()) / maxVal <= injuredHealthRatio_;
    }

    bool MagicCasterAvatarStatus::IsDamaged() const
    {
        return !onDamagedStack_.empty();
    }

    void MagicCasterAvatarStatus::AddOnDamageStack(std::unique_ptr<IDamage> damageContext)
    {
        if (invincibleRemaining_secs_ > 0.0f)
            return;

        onDamagedStack_.push(std::move(damageContext));
    }

    void MagicCasterAvatarStatus::ApplyDamage()
    {
        if (onDamagedStack_.empty())
            return;

        while (!onDamagedStack_.empty())
        {
            const auto damageContext = std::move(onDamagedStack_.front());
            onDamagedStack_.pop();
            currentHealth_->Set(StatusParameter::Health(currentHealth_->Get().Value() - damageContext->DamageValue()));
            onChangeHealth_.OnNext(currentHealth_->Get());
            event_->onDamage_.OnNext(currentHealth_->Get());
        }
        invincibleRemaining_secs_ = invincibleDuration_secs_;
    }

    void MagicCasterAvatarStatus::DiscardDamage()
    {
        std::queue<std::unique_ptr<IDamage>> empty;
        std::swap(onDamagedStack_, empty);
    }

    bool MagicCasterAvatarStatus::CanCast(const int slot, const GameCore::Magic::IMagicSpell& spell) const
    {
        if (!IsValidSpellSlot(slot))
            return false;
        if (cooldownRemaining_secs_[static_cast<size_t>(slot)] > 0.0f)
            return false;
        return mana_.Value() >= StatusParameter::Mana(spell.ManaCost());
    }

    void MagicCasterAvatarStatus::BeginCast(const int slot, const GameCore::Magic::IMagicSpell& spell)
    {
        if (!IsValidSpellSlot(slot))
            return;

        const auto remained = mana_.Value() - StatusParameter::Mana(spell.ManaCost());
        mana_.Value(remained <= StatusParameter::Mana(0.0f) ? StatusParameter::Mana(0.0f) : remained);

        cooldownRemaining_secs_[static_cast<size_t>(slot)] = spell.Cooldown_secs();
        cooldownDuration_secs_ [static_cast<size_t>(slot)] = spell.Cooldown_secs();
    }

    float MagicCasterAvatarStatus::CooldownRemaining_secs(const int slot) const
    {
        return IsValidSpellSlot(slot) ? cooldownRemaining_secs_[static_cast<size_t>(slot)] : 0.0f;
    }

    float MagicCasterAvatarStatus::CooldownRemainingRate(const int slot) const
    {
        if (!IsValidSpellSlot(slot))
            return 0.0f;

        const float duration = cooldownDuration_secs_[static_cast<size_t>(slot)];
        return duration > 0.0f ? cooldownRemaining_secs_[static_cast<size_t>(slot)] / duration : 0.0f;
    }

    void MagicCasterAvatarStatus::Heal(const StatusParameter::Health amount)
    {
        if (amount.Value() <= 0 || IsDeath())
            return;

        const int healed = (std::min)(currentHealth_->Get().Value() + amount.Value(), maxHealth_.Value());
        currentHealth_->Set(StatusParameter::Health(healed));
        onChangeHealth_.OnNext(currentHealth_->Get());
    }

    void MagicCasterAvatarStatus::RestoreStamina(const float amount)
    {
        if (amount <= 0.0f)
            return;

        const auto restored = stamina_.Value() + StatusParameter::Stamina(amount);
        stamina_.Value(maxStamina_ <= restored ? maxStamina_ : restored);
        if (isStaminaExhausted_ && stamina_.Value() >= StatusParameter::Stamina(maxStamina_.Value() * minStaminaRatioToResumeRun_))
            isStaminaExhausted_ = false;
    }

    void MagicCasterAvatarStatus::ApplyAttackBuff(const float rate, const float duration_secs)
    {
        if (rate <= 0.0f || duration_secs <= 0.0f)
            return;

        attackBuffRate_ = rate;
        attackBuffRemaining_secs_ = duration_secs;
    }

    void MagicCasterAvatarStatus::ConsumeStamina(const float cost)
    {
        const auto consumed = stamina_.Value() - StatusParameter::Stamina(cost);
        if (consumed <= StatusParameter::Stamina(0.0f))
        {
            stamina_.Value(StatusParameter::Stamina(0.0f));
            isStaminaExhausted_ = true;
        }
        else
        {
            stamina_.Value(consumed);
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
        LibCore::ImGuiHelper::OnDrawInputField("avoidRollingStateDuration_secs_", avoidRollingStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("avoidRollingStaminaCost_", avoidRollingStaminaCost_);
        LibCore::ImGuiHelper::OnDrawInputField("damageStateDuration_secs_", damageStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("invincibleDuration_secs_", invincibleDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("invincibleRemaining_secs_", invincibleRemaining_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("deathStateDuration_secs_", deathStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("injuredHealthRatio_", injuredHealthRatio_);
        LibCore::ImGuiHelper::OnDrawInputField("maxMana_", maxMana_);
        LibCore::ImGuiHelper::OnDrawInputField("mana_", mana_);
        LibCore::ImGuiHelper::OnDrawInputField("manaRegenPerSecond_", manaRegenPerSecond_);
        LibCore::ImGuiHelper::OnDrawInputField("quests_", quests_);
        ImGui::Text("attackBuff: x%.2f (%.1fs)", AttackPowerRate(), attackBuffRemaining_secs_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStatus);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::PlayerAvatar::IPlayerAvatarStatus, GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStatus);
#pragma endregion
