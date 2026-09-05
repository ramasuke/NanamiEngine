#include "SwordManAvatarStatus.h"

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../../../../Data/PlayerAvatar/InitStatus/SwordMan/Data_SwordManInitStatus.h"
#include "../../../Damage/Game_Damage_IDamage.h"
#include "Quest/SwordMan_QuestGroup.h"
#include "Event/SwordManAvatarStatusEvent.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    SwordManAvatarStatus::SwordManAvatarStatus()
        : event_ (std::make_shared<StatusEvent>())
        , quests_(std::make_unique<QuestGroup>())
        , maxHealth_(100)
        , maxStamina_(StatusParameter::Stamina(100.0f))
        , stamina_(StatusParameter::Stamina(100.0f))
        , staminaDrainPerSecond_(20.0f)
        , staminaRegenPerSecond_(10.0f)
        , minStaminaRatioToResumeRun_(0.3f)
        , comboNormalAttack_ {
            AttackParam(Damage::PhysicsPower(1), EnhancePower(1), 0.3528985507f, 0.6637681159f),
            AttackParam(Damage::PhysicsPower(2), EnhancePower(2), 0.9246376812f, 1.2855072464f),
            AttackParam(Damage::PhysicsPower(3), EnhancePower(3), 1.7f         , 2.0f         )}
        , comboNormalAttackStateDuration_secs_(2.0f)
        , attackedShockedStateDuration_secs_  (1.0f)
        , dashAttack_                     (Damage::PhysicsPower(10), EnhancePower(10), 0.7f, 0.8f)
        , walkSpeed_                      (10.0f)
        , runSpeed_                       (40.0f)
        , moveRotateSpeed_                (10.0f)
        , jumpPower_                      (32.5f)
        , jumpCooldown_secs_              (0.53f)
        , damageStateDuration_secs_       (1.6f )
        , avoidRollingStateDuration_secs_ (0.7f )
        , deathStateDuration_secs_        (2.0f )
        , downStateDuration_secs_         (15.0f)
        , reviveHealthRatio_              (0.3f )
        , injuredHealthRatio_             (0.3f )
        , wasInjured_                     (false)
    {
    }

    SwordManAvatarStatus::SwordManAvatarStatus(const Asset::SwordManInitStatus& initStatus)
        : event_                              (std::make_shared<StatusEvent>())
        , quests_                             (initStatus.Quest().DeepCoy())
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
        , walkSpeed_                          (initStatus.GetWalkSpeed())
        , runSpeed_                           (initStatus.GetRunSpeed())
        , moveRotateSpeed_                    (initStatus.GetMoveRotateSpeed())
        , jumpPower_                          (initStatus.GetJumpPower())
        , jumpCooldown_secs_                  (initStatus.GetJumpCooldown_secs())
        , damageStateDuration_secs_           (initStatus.DamageStateDuration_secs())
        , avoidRollingStateDuration_secs_     (initStatus.AvoidRollingStateDuration_secs())
        , deathStateDuration_secs_            (initStatus.DeathStateDuration_secs())
        , downStateDuration_secs_             (15.0f)
        , reviveHealthRatio_                  (0.3f )
        , injuredHealthRatio_                 (initStatus.GetInjuredHealthRatio())
        , wasInjured_                         (false)
    {
    }

    SwordManAvatarStatus::~SwordManAvatarStatus() = default;

    void SwordManAvatarStatus::Init()
    {
        quests_->Init(event_);
    }

    void SwordManAvatarStatus::OnUpdate()
    {
        if (isRunning_)
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
        }
        else
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
        }

        const bool currentlyInjured = IsInjured();
        if (currentlyInjured && !wasInjured_)
            onBecomeInjured_.get_subscriber().on_next(LibCore::Rx::unit{});
        else if (!currentlyInjured && wasInjured_)
            onRecoverFromInjured_.get_subscriber().on_next(LibCore::Rx::unit{});
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
        onDamagedStack_.push(std::move(damageContext));
    }

    void SwordManAvatarStatus::ApplyDamage()
    {
        while (!onDamagedStack_.empty())
        {
            const auto damageContext = std::move(onDamagedStack_.front());
            onDamagedStack_.pop();
            onChangeHealth_.get_subscriber().on_next(StatusParameter::Health(currentHealth_->Get().Value() - damageContext->DamageValue()));
        }
    }

    void SwordManAvatarStatus::DiscardDamage()
    {
        std::queue<std::unique_ptr<IDamage>> empty;
        std::swap(onDamagedStack_, empty);
    }

    bool SwordManAvatarStatus::IsDamaged() const
    {
        return !onDamagedStack_.empty();
    }

    void SwordManAvatarStatus::Revive()
    {
        onChangeHealth_.get_subscriber().on_next(
            StatusParameter::Health(static_cast<int>(maxHealth_.Value() * reviveHealthRatio_)));
        isDowned_ = false;
    }

    void SwordManAvatarStatus::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("quests_", quests_);
        LibCore::ImGuiHelper::OnDrawInputField("maxHealth_", maxHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("health_", currentHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("maxStamina_", maxStamina_);
        LibCore::ImGuiHelper::OnDrawInputField("stamina_", stamina_);
        LibCore::ImGuiHelper::OnDrawInputField("staminaDrainPerSecond_", staminaDrainPerSecond_);
        LibCore::ImGuiHelper::OnDrawInputField("staminaRegenPerSecond_", staminaRegenPerSecond_);
        LibCore::ImGuiHelper::OnDrawInputField("minStaminaRatioToResumeRun_", minStaminaRatioToResumeRun_);
        LibCore::ImGuiHelper::OnDrawInputField("comboNormalAttack_", comboNormalAttack_, [] {});
        LibCore::ImGuiHelper::OnDrawInputField("comboNormalAttackStateDuration_secs_", comboNormalAttackStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("attackedShockedStateDuration_secs_", attackedShockedStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("dashAttack_", dashAttack_);
        LibCore::ImGuiHelper::OnDrawInputField("walkSpeed_", walkSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("runSpeed_", runSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("moveRotateSpeed_", moveRotateSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpPower_", jumpPower_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpCooldown_secs_", jumpCooldown_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("damageStateDuration_secs_", damageStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("deathStateDuration_secs_", deathStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("downStateDuration_secs_", downStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("reviveHealthRatio_", reviveHealthRatio_);
    }
}
