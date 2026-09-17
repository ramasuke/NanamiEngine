#include "Data_SwordManInitStatus.h"

namespace NanamiEngine::Module::Asset
{
    SwordManInitStatus::SwordManInitStatus(const std::string& contentPath)
        : ScriptableObject(contentPath)
        , quests_(std::make_unique<GameCore::PlayerAvatar::SwordMan::QuestGroup>())
        , comboNormalAttack_{
            GameCore::PlayerAvatar::AttackParam(GameCore::Damage::PhysicsPower(1), GameCore::PlayerAvatar::EnhancePower(1), 0.2673473869f, 0.5028546333f),
            GameCore::PlayerAvatar::AttackParam(GameCore::Damage::PhysicsPower(2), GameCore::PlayerAvatar::EnhancePower(2), 0.7004830918f, 0.9738691261f),
            GameCore::PlayerAvatar::AttackParam(GameCore::Damage::PhysicsPower(3), GameCore::PlayerAvatar::EnhancePower(3), 1.2878787879f, 1.5151515152f)}
        , maxStamina_                         (GameCore::StatusParameter::Stamina(100.0f))
        , staminaDrainPerSecond_              (20.0f)
        , staminaRegenPerSecond_              (10.0f)
        , comboNormalAttackStateDuration_secs_(0)
        , dashAttackLungeSpeed_               (55.0f)
        , comboHitFeel_ {
            GameCore::PlayerAvatar::HitFeelParam(0.3f, 0.1090909091f, 5.0f , 0.25f, 0.12f, 30.0f),
            GameCore::PlayerAvatar::HitFeelParam(0.5f, 0.1090909091f, 5.75f, 0.35f, 0.14f, 28.0f),
            GameCore::PlayerAvatar::HitFeelParam(0.8f, 0.1090909091f, 5.7f , 0.5f , 0.18f, 40.0f)}
        , dashHitFeel_                        (0.9f, 0.1272727273f, 1.0f, 0.6f, 0.2f)
        , comboInputBufferWindow_secs_        (0.1181818182f)
        , chargeAttackHoldThreshold_secs_     (0.2f)
        , chargeAttackMaxCharge_secs_         (1.0f)
        , chargeAttackMaxHold_secs_           (3.0f)
        , chargeAttack_                       (GameCore::Damage::PhysicsPower(15), GameCore::PlayerAvatar::EnhancePower(15), 0.4333333333f, 0.9083333333f)
        , chargeHitFeel_                      (1.2f, 0.18f, 7.0f, 0.8f, 0.25f)
        , chargeAttackLungeStart_secs_        (0.0f)
        , chargeAttackLungeSpeed_             (28.0f)
        , chargeAttackStaminaCost_            (30.0f)
        , moveRotateSpeed_                    (0)
        , lockOnAttackRotateSpeed_            (10.0f)
        , attackRotateSmoothTime_secs_        (0.08f)
        , jumpPower_                          (0)
        , jumpStateDuration_secs_             (0)
        , jumpCooldown_secs_                  (0)
        , jumpStaminaCost_                    (15.0f)
        , damageStateDuration_secs_           (0)
        , avoidRollingStateDuration_secs_     (0)
        , avoidRollingStaminaCost_            (20.0f)
        , deathStateDuration_secs_            (0)
    {
    }

    void SwordManInitStatus::OnDrawGui()
    {
        if (ImGui::Button("CreateQuest"))
        {
            quests_ = std::make_unique<GameCore::PlayerAvatar::SwordMan::QuestGroup>();
        }
        LibCore::ImGuiHelper::OnDrawInputField("maxHealth_", maxHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("minHealth_", minHealth_);
        LibCore::ImGuiHelper::OnDrawInputField("health_", health_);
        LibCore::ImGuiHelper::OnDrawInputField("maxStamina_", maxStamina_);
        LibCore::ImGuiHelper::OnDrawInputField("staminaDrainPerSecond_", staminaDrainPerSecond_);
        LibCore::ImGuiHelper::OnDrawInputField("staminaRegenPerSecond_", staminaRegenPerSecond_);
        LibCore::ImGuiHelper::OnDrawInputField("minStaminaRatioToResumeRun_", minStaminaRatioToResumeRun_);
        LibCore::ImGuiHelper::OnDrawInputField("comboNormalAttack_", comboNormalAttack_, [] {});
        LibCore::ImGuiHelper::OnDrawInputField("comboNormalAttackStateDuration_secs_", comboNormalAttackStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("attackedShockedStateDuration_secs_", attackedShockedStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("dashAttack_", dashAttack_);
        LibCore::ImGuiHelper::OnDrawInputField("dashAttackLungeSpeed_", dashAttackLungeSpeed_);
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
        LibCore::ImGuiHelper::OnDrawInputField("walkSpeed_", walkSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("runSpeed_", runSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("moveRotateSpeed_", moveRotateSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("lockOnAttackRotateSpeed_", lockOnAttackRotateSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("attackRotateSmoothTime_secs_", attackRotateSmoothTime_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpPower_", jumpPower_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpStateDuration_secs_", jumpStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpCooldown_secs_", jumpCooldown_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpStaminaCost_", jumpStaminaCost_);
        LibCore::ImGuiHelper::OnDrawInputField("damageStateDuration_secs_", damageStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("deathStateDuration_secs_", deathStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("avoidRollingStateDuration_secs_", avoidRollingStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("avoidRollingStaminaCost_", avoidRollingStaminaCost_);
        LibCore::ImGuiHelper::OnDrawInputField("initialMoney_", initialMoney_);
    }
}
