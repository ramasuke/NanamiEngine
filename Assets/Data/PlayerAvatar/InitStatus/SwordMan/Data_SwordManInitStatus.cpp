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
        , comboNormalAttackStateDuration_secs_(0)
        , moveRotateSpeed_                    (0)
        , jumpPower_                          (0)
        , jumpCooldown_secs_                  (0)
        , onEnableReinforceDuration_secs_     (0)
        , onDisableReinforceDuration_secs_    (0)
        , damageStateDuration_secs_           (0)
        , avoidRollingStateDuration_secs_     (0)
        , deathStateDuration_secs_            (0)
        , reinforceModeDuration_secs_         (0)
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
        LibCore::ImGuiHelper::OnDrawInputField("reinforceRequireEnhance_", reinforceRequireEnhance_);
        LibCore::ImGuiHelper::OnDrawInputField("damageStateDuration_secs_", damageStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("deathStateDuration_secs_", deathStateDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("avoidRollingStateDuration_secs_", avoidRollingStateDuration_secs_);
    }
}
