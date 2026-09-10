#include "Data_SwordManInitStatus.h"

namespace NanamiEngine::Module::Asset
{
    SwordManInitStatus::SwordManInitStatus(const std::string& contentPath)
        : ScriptableObject(contentPath)
        , quests_(std::make_unique<GameCore::PlayerAvatar::SwordMan::QuestGroup>())
        , comboNormalAttack_{
            GameCore::PlayerAvatar::AttackParam(GameCore::Damage::PhysicsPower(1), GameCore::PlayerAvatar::EnhancePower(1), 0.3528985507f, 0.6637681159f),
            GameCore::PlayerAvatar::AttackParam(GameCore::Damage::PhysicsPower(2), GameCore::PlayerAvatar::EnhancePower(2), 0.9246376812f, 1.2855072464f),
            GameCore::PlayerAvatar::AttackParam(GameCore::Damage::PhysicsPower(3), GameCore::PlayerAvatar::EnhancePower(3), 1.7f         , 2.0f         )}
        , maxStamina_                         (GameCore::StatusParameter::Stamina(100.0f))
        , staminaDrainPerSecond_              (20.0f)
        , staminaRegenPerSecond_              (10.0f)
        , comboNormalAttackStateDuration_secs_(0)
        , dashAttackLungeSpeed_               (55.0f)
        , comboHitFeel_ {
            GameCore::PlayerAvatar::HitFeelParam(0.04f, 0.5f , 0.3f, 0.12f, 5.0f ),
            GameCore::PlayerAvatar::HitFeelParam(0.05f, 0.4f , 0.5f, 0.12f, 5.75f),
            GameCore::PlayerAvatar::HitFeelParam(0.08f, 0.15f, 0.8f, 0.12f, 5.7f)}
        , dashHitFeel_                        (0.09f, 0.1f, 0.9f, 0.14f, 1.0f)
        , comboInputBufferWindow_secs_        (0.13f)
        , moveRotateSpeed_                    (0)
        , lockOnAttackRotateSpeed_            (3.0f)
        , jumpPower_                          (0)
        , jumpCooldown_secs_                  (0)
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
        LibCore::ImGuiHelper::OnDrawInputField("walkSpeed_", walkSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("runSpeed_", runSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("moveRotateSpeed_", moveRotateSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("lockOnAttackRotateSpeed_", lockOnAttackRotateSpeed_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpPower_", jumpPower_);
        LibCore::ImGuiHelper::OnDrawInputField("jumpCooldown_secs_", jumpCooldown_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("damageStateDuration_secs_", damageStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("deathStateDuration_secs_", deathStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("avoidRollingStateDuration_secs_", avoidRollingStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("avoidRollingStaminaCost_", avoidRollingStaminaCost_);
    }
}
