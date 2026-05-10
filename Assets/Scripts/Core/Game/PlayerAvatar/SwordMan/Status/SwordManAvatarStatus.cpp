#include "SwordManAvatarStatus.h"

#include "../../../../../../../Engine/Core/Application/Time/Time.h"
#include "../../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../../../../Data/PlayerAvatarInitStatus/SwordMan/Data_SwordManInitStatus.h"
#include "../../../DamageContext/IDamageContext.h"
#include "Quest/SwordMan_QuestGroup.h"
#include "Event/SwordManAvatarStatusEvent.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    SwordManAvatarStatus::SwordManAvatarStatus()
        : event_ (std::make_shared<StatusEvent>())
        , quests_(std::make_unique<QuestGroup>())
        , maxHealth_(100)
        , health_(StatusParameter::Health(100))
        , maxEnhancePowerStack_(30)
        , enhancePowerStack_(EnhancePower(0))
        , comboNormalAttack_ {
            AttackParam(Damage::PhysicsPower(1), EnhancePower(1), 0.3528985507f, 0.6637681159f),
            AttackParam(Damage::PhysicsPower(2), EnhancePower(2), 0.9246376812f, 1.2855072464f),
            AttackParam(Damage::PhysicsPower(3), EnhancePower(3), 1.7f         , 2.0f         )}
        , comboNormalAttackStateDuration_secs_(2.0f)
        , dashAttack_                     (Damage::PhysicsPower(10), EnhancePower(10), 0.7f, 0.8f)
        , walkSpeed_                      (10.0f)
        , runSpeed_                       (40.0f)
        , moveRotateSpeed_                (10.0f)
        , jumpPower_                      (32.5f)
        , jumpCooldown_secs_              (0.53f)
        , onEnableReinforceDuration_secs_ (0.5f )
        , onDisableReinforceDuration_secs_(0.5f )
        , reinforceRequireEnhance_        (30   )
        , damageStateDuration_secs_       (1.6f )
        , avoidRollingStateDuration_secs_ (0.7f )
        , deathStateDuration_secs_        (2.0f )
        , reinforceModeDuring_secs_       (0.0f )
        , reinforceModeDuration_secs_     (10.0f)
        , isReinforceMode_                (false)
    {
    }

    SwordManAvatarStatus::SwordManAvatarStatus(const Asset::SwordManInitStatus& initStatus)
        : event_                              (std::make_shared<StatusEvent>())
        , quests_                             (initStatus.Quest().DeepCoy())
        , maxHealth_                          (initStatus.MaxHealth())
        , minHealth_                          (initStatus.MinHealth())
        , health_                             (initStatus.Health())
        , maxEnhancePowerStack_               (initStatus.MaxEnhancePowerStack())
        , enhancePowerStack_                  (initStatus.EnhancePowerStack())
        , decrementEnhancePowerStack_secs_    (initStatus.DecrementEnhancePowerStack_secs())
        , comboNormalAttack_                  (initStatus.ComboNormalAttack())
        , comboNormalAttackStateDuration_secs_(initStatus.ComboNormalAttackStateDuration_secs())
        , dashAttack_                         (initStatus.DashAttack())
        , walkSpeed_                          (initStatus.GetWalkSpeed())
        , runSpeed_                           (initStatus.GetRunSpeed())
        , moveRotateSpeed_                    (initStatus.GetMoveRotateSpeed())
        , jumpPower_                          (initStatus.GetJumpPower())
        , jumpCooldown_secs_                  (initStatus.GetJumpCooldown_secs())
        , onEnableReinforceDuration_secs_     (initStatus.OnEnableReinforceDuration_secs())
        , onDisableReinforceDuration_secs_    (initStatus.OnDisableReinforceDuration_secs())
        , reinforceRequireEnhance_            (initStatus.ReinforceRequireEnhance())
        , damageStateDuration_secs_           (initStatus.DamageStateDuration_secs())
        , avoidRollingStateDuration_secs_     (initStatus.AvoidRollingStateDuration_secs())
        , deathStateDuration_secs_            (initStatus.DeathStateDuration_secs())
        , reinforceModeDuring_secs_           (0)
        , reinforceModeDuration_secs_         (initStatus.ReinforceModeDuration_secs())
    {
    }

    SwordManAvatarStatus::~SwordManAvatarStatus() = default;

    void SwordManAvatarStatus::Init()
    {
        quests_->Init(event_);
    }

    void SwordManAvatarStatus::OnUpdate()
    {
        if (isReinforceMode_.get())
        {
            reinforceModeDuring_secs_ += Time::DeltaTime();
        }
    }

    void SwordManAvatarStatus::OnEnableReinforce()
    {
        enhancePowerStack_.OnNext(EnhancePower(0));
        isReinforceMode_.OnNext(true);
    }

    void SwordManAvatarStatus::OnDisableReinforce()
    {
        reinforceModeDuring_secs_ = 0.0f;
        isReinforceMode_.OnNext(false);
    }

    void SwordManAvatarStatus::AddEnhancePowerStack(const EnhancePower& enhancePower)
    {
        const auto addedEnhancePowerStackCount = enhancePowerStack_.get() + enhancePower;
        if (maxEnhancePowerStack_ < addedEnhancePowerStackCount)
        {
            enhancePowerStack_.OnNext(maxEnhancePowerStack_);
            return;
        }
        enhancePowerStack_.OnNext(addedEnhancePowerStackCount);
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
            health_.OnNext(StatusParameter::Health(health_.get().Value() - damageContext->DamageValue()));
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

    void SwordManAvatarStatus::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("quests_", quests_);
        LibCore::ImGuiHelper::OnDrawInputField("maxHealth_", maxHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("health_", health_);
        LibCore::ImGuiHelper::OnDrawInputField("maxEnhancePowerStack_", maxEnhancePowerStack_);
        LibCore::ImGuiHelper::OnDrawInputField("enhancePowerStack_", enhancePowerStack_);
        LibCore::ImGuiHelper::OnDrawInputField("comboNormalAttack_", comboNormalAttack_, [] {});
        LibCore::ImGuiHelper::OnDrawInputField("comboNormalAttackStateDuration_secs_", comboNormalAttackStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("dashAttack_", dashAttack_);
        LibCore::ImGuiHelper::OnDrawInputField("walkSpeed_", walkSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("runSpeed_", runSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("moveRotateSpeed_", moveRotateSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpPower_", jumpPower_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpCooldown_secs_", jumpCooldown_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("onEnableReinforceDuration_secs_", onEnableReinforceDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("isReinforceMode_", isReinforceMode_);
        LibCore::ImGuiHelper::OnDrawInputField("reinforceRequireEnhance_", reinforceRequireEnhance_);
        LibCore::ImGuiHelper::OnDrawInputField("damageStateDuration_secs_", damageStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("deathStateDuration_secs_", deathStateDuration_secs_);
    }
}
