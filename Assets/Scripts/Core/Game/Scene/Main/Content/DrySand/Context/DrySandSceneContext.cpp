#include "DrySandSceneContext.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

void GameCore::Scene::DrySandSceneContext::Init()
{
    SceneContextBase::Init();

    bgm_.Init();
    networkRunner_.Init();
    enemySpawnPointsRoot_.Init();
    arrivalCamera_.Init();
    cameraBrain_.Init();
    arrivalPortalPrefab_.Init();
    floatingStone_.Init();
}

std::vector<std::shared_ptr<GameCore::Npc::Enemy::EnemySpawnPoint>>
GameCore::Scene::DrySandSceneContext::EnemySpawnPoints() const
{
    std::vector<std::shared_ptr<Npc::Enemy::EnemySpawnPoint>> spawnPoints;
    for (const auto& child : enemySpawnPointsRoot_->Transform().GetAllChildren())
    {
        if (const auto spawnPoint = child->Components().Catch<Npc::Enemy::EnemySpawnPoint>().lock())
            spawnPoints.push_back(spawnPoint);
    }
    return spawnPoints;
}

std::optional<GameCore::Story::StageClearCondition> GameCore::Scene::DrySandSceneContext::StageClear() const
{
    if (clearEnemyKind_ < 0 || clearStoryFlag_ < 0)
        return std::nullopt;
    return Story::StageClearCondition{
        static_cast<Npc::Enemy::EnemyKind>(clearEnemyKind_),
        static_cast<Story::StoryFlag>(clearStoryFlag_) };
}

void GameCore::Scene::DrySandSceneContext::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("bgm_", bgm_);
    ImGuiHelper::OnDrawInputField("networkRunner_", networkRunner_);
    ImGuiHelper::OnDrawInputField("enemySpawnPointsRoot_", enemySpawnPointsRoot_);
    ImGuiHelper::OnDrawInputField("arrivalCamera_", arrivalCamera_);
    ImGuiHelper::OnDrawInputField("cameraBrain_", cameraBrain_);
    ImGuiHelper::OnDrawInputField("arrivalPortalPrefab_", arrivalPortalPrefab_);
    ImGuiHelper::OnDrawInputField("arrivalPortalOpenDelay_msecs_", arrivalPortalOpenDelay_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalPortalOpen_msecs_", arrivalPortalOpen_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalWalk_msecs_", arrivalWalk_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalPortalCloseDelay_msecs_", arrivalPortalCloseDelay_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalPortalClose_msecs_", arrivalPortalClose_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalHold_msecs_", arrivalHold_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalPortalHeight_", arrivalPortalHeight_);
    ImGuiHelper::OnDrawInputField("arrivalWalkStartBehind_", arrivalWalkStartBehind_);

    bool hasStageClear = clearEnemyKind_ >= 0 && clearStoryFlag_ >= 0;
    if (ImGui::Checkbox("stageClear", &hasStageClear))
    {
        clearEnemyKind_ = hasStageClear ? static_cast<int>(Npc::Enemy::EnemyKind::NormalBoss) : -1;
        clearStoryFlag_ = hasStageClear ? 0 : -1;
    }
    if (hasStageClear)
    {
        auto kind = static_cast<Npc::Enemy::EnemyKind>(clearEnemyKind_);
        ImGuiHelper::OnDrawEnumField("clearEnemyKind_", kind, Npc::Enemy::ENEMY_KINDS, Npc::Enemy::ToString);
        clearEnemyKind_ = static_cast<int>(kind);

        auto flag = static_cast<Story::StoryFlag>(clearStoryFlag_);
        ImGuiHelper::OnDrawEnumField("clearStoryFlag_", flag, Story::STORY_FLAGS, Story::ToString);
        clearStoryFlag_ = static_cast<int>(flag);
    }
    ImGuiHelper::OnDrawInputField("arrivalWalkDistance_", arrivalWalkDistance_);
    ImGuiHelper::OnDrawInputField("arrivalCameraStart_", arrivalCameraStart_);
    ImGuiHelper::OnDrawInputField("arrivalCameraEnd_", arrivalCameraEnd_);
    ImGuiHelper::OnDrawInputField("arrivalLookAtHeight_", arrivalLookAtHeight_);
    ImGuiHelper::OnDrawInputField("floatingStone_", floatingStone_);
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Scene::DrySandSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::SceneContextBase, GameCore::Scene::DrySandSceneContext);
#pragma endregion
