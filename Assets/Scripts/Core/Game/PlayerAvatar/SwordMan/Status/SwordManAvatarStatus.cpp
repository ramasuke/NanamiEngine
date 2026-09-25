#include "SwordManAvatarStatus.h"

#include <algorithm>
#include <cassert>

#include "Engine/Core/Application/Time/Time.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../../../../Data/PlayerAvatar/InitStatus/SwordMan/Data_SwordManInitStatus.h"
#include "../../../Damage/Game_Damage_IDamage.h"
#include "Quest/SwordMan_QuestGroup.h"
#include "Event/SwordManAvatarStatusEvent.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    SwordManAvatarStatus::SwordManAvatarStatus()
        : event_ (std::make_shared<StatusEvent>())
        , quests_(std::make_unique<QuestGroup>())
        , wallet_(std::make_shared<PlayerAvatar::Wallet>())
        , maxHealth_(150)
        , currentHealth_(SyncParamFactory::Create<StatusParameter::Health>(this, maxHealth_))
        , maxStamina_(StatusParameter::Stamina(100.0f))
        , stamina_(StatusParameter::Stamina(100.0f))
        , staminaDrainPerSecond_(10.0f)
        , staminaRegenPerSecond_(30.0f)
        , minStaminaRatioToResumeRun_(0.3f)
        , comboNormalAttack_ {
            AttackParam(Damage::PhysicsPower(10), EnhancePower(1), 0.2673473869f, 0.5028546333f),
            AttackParam(Damage::PhysicsPower(12), EnhancePower(2), 0.7004830918f, 0.9738691261f),
            AttackParam(Damage::PhysicsPower(18), EnhancePower(3), 1.2878787879f, 1.5151515152f)}
        , comboNormalAttackStateDuration_secs_(1.5151515152f)
        , attackedShockedStateDuration_secs_  (0.9090909091f)
        , dashAttack_                    (Damage::PhysicsPower(15), EnhancePower(10), 0.5303030303f, 0.6060606061f)
        , dashAttackLungeSpeed_secs_          (55.0f)
        , comboHitFeel_ {
            HitFeelParam(0.3f, 0.1090909091f, 5.0f , 0.5f, 0.12f, 30.0f),
            HitFeelParam(0.5f, 0.1090909091f, 5.75f, 0.7f, 0.14f, 28.0f),
            HitFeelParam(0.8f, 0.1090909091f, 6.75f, 1.0f , 0.18f, 40.0f)}
        , dashHitFeel_                   (0.9f, 0.1272727273f, 1.0f, 0.6f, 0.2f)
        , comboInputBufferWindow_secs_   (0.1181818182f)
        , chargeAttackHoldThreshold_secs_(0.2f)
        , chargeAttackMaxCharge_secs_    (1.0f)
        , chargeAttackMaxHold_secs_      (3.0f)
        , chargeAttack_                  (Damage::PhysicsPower(35), EnhancePower(15), 0.4333333333f, 0.9083333333f)
        , chargeHitFeel_                 (1.2f, 0.18f, 7.0f, 0.8f, 0.25f)
        , chargeAttackLungeStart_secs_   (0.0f)
        , chargeAttackLungeSpeed_        (28.0f)
        , chargeAttackStaminaCost_       (30.0f)
        , jumpAttack_                    (Damage::PhysicsPower(22), EnhancePower(12), 0.1f, 0.6666666667f)
        , jumpAttackHitFeel_             (1.0f, 0.15f, 6.5f, 0.8f, 0.22f)
        , jumpAttackWindup_secs_         (0.3f)
        , jumpAttackPlungeSpeed_         (120.0f)
        , walkSpeed_                    (24.0f)
        , runSpeed_                      (70.0f)
        , moveRotateSpeed_               (6.2f)
        , lockOnAttackRotateSpeed_       (10.0f)
        , attackRotateSmoothTime_secs_   (0.08f)
        , jumpPower_                     (75.5f)
        , jumpStateDuration_secs_        (0.4818181818f)
        , jumpCooldown_secs_             (0.58f )
        , jumpStaminaCost_               (15.0f)
        , damageStateDuration_secs_      (1.4545454545f)
        , avoidRollingStateDuration_secs_(0.6363636364f)
        , avoidRollingStaminaCost_       (20.0f)
        , deathStateDuration_secs_       (1.8181818182f)
        , downStateDuration_secs_        (13.6363636364f)
        , fallDownStateDuration_secs_    (1.3333333333f)
        , getUpStateDuration_secs_       (1.7666666667f)
        , reviveHealthRatio_             (0.3f )
        , injuredHealthRatio_            (0.3f )
        , wasInjured_                    (false)
    {
    }

    SwordManAvatarStatus::SwordManAvatarStatus(const Asset::SwordManInitStatus& initStatus)
        : event_                              (std::make_shared<StatusEvent>())
        , quests_                             (initStatus.Quest().DeepCoy())
        , wallet_                             (std::make_shared<PlayerAvatar::Wallet>(initStatus.InitialMoney()))
        , maxHealth_                          (initStatus.MaxHealth())
        , minHealth_                          (initStatus.MinHealth())
        , currentHealth_                      (SyncParamFactory::Create<StatusParameter::Health>(this, initStatus.Health()))
        , maxStamina_                         (initStatus.MaxStamina())
        , stamina_                            (initStatus.MaxStamina())
        , staminaDrainPerSecond_              (initStatus.StaminaDrainPerSecond())
        , staminaRegenPerSecond_              (initStatus.StaminaRegenPerSecond())
        , minStaminaRatioToResumeRun_         (initStatus.MinStaminaRatioToResumeRun())
        , comboNormalAttack_                  (initStatus.ComboNormalAttack())
        , comboNormalAttackStateDuration_secs_(initStatus.ComboNormalAttackStateDuration_secs())
        , attackedShockedStateDuration_secs_  (initStatus.AttackedShockedStateDuration_secs_())
        , dashAttack_                         (initStatus.DashAttack())
        , dashAttackLungeSpeed_secs_               (initStatus.GetDashAttackLungeSpeed())
        , comboHitFeel_                       (initStatus.ComboHitFeel())
        , dashHitFeel_                        (initStatus.DashHitFeel())
        , comboInputBufferWindow_secs_        (initStatus.GetComboInputBufferWindow_secs())
        , chargeAttackHoldThreshold_secs_     (initStatus.ChargeAttackHoldThreshold_secs())
        , chargeAttackMaxCharge_secs_         (initStatus.ChargeAttackMaxCharge_secs())
        , chargeAttackMaxHold_secs_           (initStatus.ChargeAttackMaxHold_secs())
        , chargeAttack_                       (initStatus.ChargeAttack())
        , chargeHitFeel_                      (initStatus.ChargeHitFeel())
        , chargeAttackLungeStart_secs_        (initStatus.ChargeAttackLungeStart_secs())
        , chargeAttackLungeSpeed_             (initStatus.ChargeAttackLungeSpeed())
        , chargeAttackStaminaCost_            (initStatus.ChargeAttackStaminaCost())
        , jumpAttack_                         (Damage::PhysicsPower(6), EnhancePower(12), 0.1f, 0.6666666667f)
        , jumpAttackHitFeel_                  (1.5f, 0.15f, 6.5f, 0.8f, 0.22f)
        , jumpAttackWindup_secs_              (0.3f)
        , jumpAttackPlungeSpeed_              (120.0f)
        , walkSpeed_                          (initStatus.GetWalkSpeed())
        , runSpeed_                           (initStatus.GetRunSpeed())
        , moveRotateSpeed_                    (initStatus.GetMoveRotateSpeed())
        , lockOnAttackRotateSpeed_            (initStatus.GetLockOnAttackRotateSpeed())
        , attackRotateSmoothTime_secs_        (initStatus.AttackRotateSmoothTime_secs())
        , jumpPower_                          (initStatus.GetJumpPower())
        , jumpStateDuration_secs_             (initStatus.GetJumpStateDuration_secs())
        , jumpCooldown_secs_                  (initStatus.JumpCooldown_secs())
        , jumpStaminaCost_                    (initStatus.JumpStaminaCost())
        , damageStateDuration_secs_           (initStatus.DamageStateDuration_secs())
        , avoidRollingStateDuration_secs_     (initStatus.AvoidRollingStateDuration_secs())
        , avoidRollingStaminaCost_            (initStatus.AvoidRollingStaminaCost())
        , deathStateDuration_secs_            (initStatus.DeathStateDuration_secs())
        , downStateDuration_secs_             (13.6363636364f)
        , fallDownStateDuration_secs_         (1.3333333333f)
        , getUpStateDuration_secs_            (1.7666666667f)
        , reviveHealthRatio_                  (0.3f )
        , injuredHealthRatio_                 (initStatus.GetInjuredHealthRatio())
        , wasInjured_                         (false)
    {
    }

    SwordManAvatarStatus::~SwordManAvatarStatus() = default;

    void SwordManAvatarStatus::Init()
    {
        quests_->Init(event_, controlGuideFocus_, wallet_);
    }

    void SwordManAvatarStatus::OnUpdate()
    {
        if (jumpCooldownRemaining_secs_ > 0.0f)
        {
            jumpCooldownRemaining_secs_ -= Time::DeltaTime();
            jumpCooldownRemaining_secs_ = (std::max)(jumpCooldownRemaining_secs_, 0.0f);
        }

        if (attackBuffRemaining_secs_ > 0.0f)
        {
            attackBuffRemaining_secs_ -= Time::DeltaTime();
            attackBuffRemaining_secs_ = (std::max)(attackBuffRemaining_secs_, 0.0f);
        }

        if (invincibleRemaining_secs_ > 0.0f)
            invincibleRemaining_secs_ = (std::max)(invincibleRemaining_secs_ - Time::DeltaTime(), 0.0f);

        assert(stateMachine_ && "SwordManAvatarStatus: stateMachine_ is not set");
        switch (stateMachine_->GetCurrentStateType())
        {
        case SwordManAvatarStateType::Run:
        case SwordManAvatarStateType::InjuredRun:
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
        case SwordManAvatarStateType::AvoidRolling:
        case SwordManAvatarStateType::Jump:
        case SwordManAvatarStateType::Floating:
        case SwordManAvatarStateType::JumpAttackAir:
        case SwordManAvatarStateType::JumpAttackLand:
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
            onBecomeInjured_.OnNext(R4::Unit{});
        else if (!currentlyInjured && wasInjured_)
            onRecoverFromInjured_.OnNext(R4::Unit{});
        wasInjured_ = currentlyInjured;
    }

    bool SwordManAvatarStatus::IsInjured() const
    {
        const auto maxVal = static_cast<float>(maxHealth_.Value());
        if (maxVal <= 0.0f) return false;
        return static_cast<float>(currentHealth_->Get().Value()) / maxVal <= injuredHealthRatio_;
    }

    void SwordManAvatarStatus::AddOnDamageStack(std::unique_ptr<IDamage> damageContext)
    {
        if (invincibleRemaining_secs_ > 0.0f)
            return;

        onDamagedStack_.push(std::move(damageContext));
    }

    void SwordManAvatarStatus::ApplyDamage()
    {
        if (onDamagedStack_.empty())
            return;

        while (!onDamagedStack_.empty())
        {
            const auto damageContext = std::move(onDamagedStack_.front());
            onDamagedStack_.pop();
            currentHealth_->Set(StatusParameter::Health(currentHealth_->Get().Value() - damageContext->DamageValue()));
            onChangeHealth_.OnNext(currentHealth_->Get());
        }
        invincibleRemaining_secs_ = invincibleDuration_secs_;
    }

    void SwordManAvatarStatus::DiscardDamage()
    {
        std::queue<std::unique_ptr<IDamage>> empty;
        std::swap(onDamagedStack_, empty);
    }

    void SwordManAvatarStatus::ConsumeAvoidRollingStamina()
    {
        ConsumeStamina(avoidRollingStaminaCost_);
    }

    void SwordManAvatarStatus::ConsumeChargeAttackStamina()
    {
        ConsumeStamina(chargeAttackStaminaCost_);
    }

    void SwordManAvatarStatus::ConsumeJumpStamina()
    {
        ConsumeStamina(jumpStaminaCost_);
    }

    void SwordManAvatarStatus::ConsumeStamina(const float cost)
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

    void SwordManAvatarStatus::StartJumpCooldown()
    {
        jumpCooldownRemaining_secs_ = jumpCooldown_secs_;
    }

    bool SwordManAvatarStatus::IsDamaged() const
    {
        return !onDamagedStack_.empty();
    }

    void SwordManAvatarStatus::Revive()
    {
        currentHealth_->Set(StatusParameter::Health(static_cast<int>(maxHealth_.Value() * reviveHealthRatio_)));
        onChangeHealth_.OnNext(currentHealth_->Get());
        isDowned_ = false;
    }

    void SwordManAvatarStatus::Heal(const StatusParameter::Health amount)
    {
        if (amount.Value() <= 0 || IsDeath())
            return;

        const int healed = (std::min)(currentHealth_->Get().Value() + amount.Value(), maxHealth_.Value());
        currentHealth_->Set(StatusParameter::Health(healed));
        onChangeHealth_.OnNext(currentHealth_->Get());
    }

    void SwordManAvatarStatus::RestoreStamina(const float amount)
    {
        if (amount <= 0.0f)
            return;

        const auto restored = stamina_.Value() + StatusParameter::Stamina(amount);
        stamina_.Value(maxStamina_ <= restored ? maxStamina_ : restored);
        if (isStaminaExhausted_ && stamina_.Value() >= StatusParameter::Stamina(maxStamina_.Value() * minStaminaRatioToResumeRun_))
            isStaminaExhausted_ = false;
    }

    void SwordManAvatarStatus::ApplyAttackBuff(const float rate, const float duration_secs)
    {
        if (rate <= 0.0f || duration_secs <= 0.0f)
            return;

        attackBuffRate_ = rate;
        attackBuffRemaining_secs_ = duration_secs;
    }

    void SwordManAvatarStatus::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("quests_", quests_);
        LibCore::ImGuiHelper::OnDrawInputField("wallet_", wallet_);
        LibCore::ImGuiHelper::OnDrawInputField("maxHealth_", maxHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("health_", currentHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("maxStamina_", maxStamina_);
        LibCore::ImGuiHelper::OnDrawInputField("stamina_", stamina_);
        LibCore::ImGuiHelper::OnDrawInputField("staminaDrainPerSecond_", staminaDrainPerSecond_);
        LibCore::ImGuiHelper::OnDrawInputField("staminaRegenPerSecond_", staminaRegenPerSecond_);
        LibCore::ImGuiHelper::OnDrawInputField("minStaminaRatioToResumeRun_", minStaminaRatioToResumeRun_);
        LibCore::ImGuiHelper::OnDrawInputField("avoidRollingStaminaCost_", avoidRollingStaminaCost_);
        LibCore::ImGuiHelper::OnDrawInputField("comboNormalAttack_", comboNormalAttack_, [] {});
        LibCore::ImGuiHelper::OnDrawInputField("comboNormalAttackStateDuration_secs_", comboNormalAttackStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("attackedShockedStateDuration_secs_", attackedShockedStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("dashAttack_", dashAttack_);
        LibCore::ImGuiHelper::OnDrawInputField("dashAttackLungeSpeed_", dashAttackLungeSpeed_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("comboHitFeel_", comboHitFeel_, [] {});
        LibCore::ImGuiHelper::OnDrawInputField("dashHitFeel_", dashHitFeel_);
        LibCore::ImGuiHelper::OnDrawInputField("comboInputBufferWindow_secs_", comboInputBufferWindow_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("chargeAttackHoldThreshold_secs_", chargeAttackHoldThreshold_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("chargeAttackMaxCharge_secs_", chargeAttackMaxCharge_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("chargeAttackMaxHold_secs_", chargeAttackMaxHold_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("chargeAttack_", chargeAttack_);
        LibCore::ImGuiHelper::OnDrawInputField("chargeHitFeel_", chargeHitFeel_);
        LibCore::ImGuiHelper::OnDrawInputField("chargeAttackLungeStart_secs_", chargeAttackLungeStart_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("chargeAttackLungeSpeed_", chargeAttackLungeSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("chargeAttackStaminaCost_", chargeAttackStaminaCost_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpAttack_", jumpAttack_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpAttackHitFeel_", jumpAttackHitFeel_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpAttackWindup_secs_", jumpAttackWindup_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpAttackPlungeSpeed_", jumpAttackPlungeSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("walkSpeed_", walkSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("runSpeed_", runSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("moveRotateSpeed_", moveRotateSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("lockOnAttackRotateSpeed_", lockOnAttackRotateSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("attackRotateSmoothTime_secs_", attackRotateSmoothTime_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpPower_", jumpPower_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpStateDuration_secs_", jumpStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpCooldown_secs_", jumpCooldown_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpCooldownRemaining_secs_", jumpCooldownRemaining_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpStaminaCost_", jumpStaminaCost_);
        LibCore::ImGuiHelper::OnDrawInputField("damageStateDuration_secs_", damageStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("invincibleDuration_secs_", invincibleDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("invincibleRemaining_secs_", invincibleRemaining_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("deathStateDuration_secs_", deathStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("downStateDuration_secs_", downStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("fallDownStateDuration_secs_", fallDownStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("getUpStateDuration_secs_", getUpStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("reviveHealthRatio_", reviveHealthRatio_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus, GameCore::PlayerAvatar::IPlayerAvatarStatus);
#pragma endregion
