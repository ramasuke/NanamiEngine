#include "DragonNestSceneContext.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

void GameCore::Scene::DragonNestSceneContext::Init()
{
    SceneContextBase::Init();

    bgm_.Init();
    networkRunner_.Init();
    enemySpawnPointsRoot_.Init();
    arrivalCamera_.Init();
    cameraBrain_.Init();
    arrivalPortalPrefab_.Init();
    arrivalCaptionPrefab_.Init();
    arrivalOverviewStartCamera_.Init();
    arrivalOverviewEndCamera_.Init();
    heartMoundCenterPos_.Init();
    for (auto& shot : arrivalTourShots_)
        shot.Init();
    heartsRoot_.Init();
    floatingRoot_.Init();
    endingCamera_.Init();
    greenHeartTrail_.Init();
    lightHeartTrail_.Init();
    fireHeartTrail_.Init();
    heartBurst_.Init();
}

glm::vec3 GameCore::Scene::DragonNestSceneContext::HeartMoundCenter() const
{
    return heartMoundCenterPos_->Transform().GetWorldPos();
}

std::vector<std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>>
GameCore::Scene::DragonNestSceneContext::ScatterHearts() const
{
    std::vector<std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>> hearts;
    if (const auto root = heartsRoot_.get())
    {
        for (const auto& child : root->Transform().GetChildren())
            hearts.push_back(child);
    }
    if (const auto root = floatingRoot_.get())
    {
        for (const auto& child : root->Transform().GetChildren())
        {
            if (child->Name().starts_with("NestHeart"))
                hearts.push_back(child);
        }
    }
    return hearts;
}

std::shared_ptr<NanamiEngine::Module::Asset::PrefabGameObjectFile>
GameCore::Scene::DragonNestSceneContext::HeartTrail(const std::string& heartName) const
{
    if (heartName.find("Green") != std::string::npos)
        return greenHeartTrail_.get();
    if (heartName.find("Fire") != std::string::npos)
        return fireHeartTrail_.get();
    return lightHeartTrail_.get();
}

std::vector<std::shared_ptr<GameCore::Npc::Enemy::EnemySpawnPoint>>
GameCore::Scene::DragonNestSceneContext::EnemySpawnPoints() const
{
    std::vector<std::shared_ptr<Npc::Enemy::EnemySpawnPoint>> spawnPoints;
    for (const auto& child : enemySpawnPointsRoot_->Transform().GetAllChildren())
    {
        if (const auto spawnPoint = child->Components().Catch<Npc::Enemy::EnemySpawnPoint>().lock())
            spawnPoints.push_back(spawnPoint);
    }
    return spawnPoints;
}

std::optional<GameCore::Story::StageClearCondition> GameCore::Scene::DragonNestSceneContext::StageClear() const
{
    if (clearEnemyKind_ < 0 || clearStoryFlag_ < 0)
        return std::nullopt;
    return Story::StageClearCondition{
        static_cast<Npc::Enemy::EnemyKind>(clearEnemyKind_),
        static_cast<Story::StoryFlag>(clearStoryFlag_) };
}

void GameCore::Scene::DragonNestSceneContext::OnDrawGui()
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
    ImGuiHelper::OnDrawInputField("arrivalOverview_msecs_", arrivalOverview_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalOverviewDescend_msecs_", arrivalOverviewDescend_msecs_);
    ImGuiHelper::OnDrawInputField("arrivalOverviewStartCamera_", arrivalOverviewStartCamera_);
    ImGuiHelper::OnDrawInputField("arrivalOverviewEndCamera_", arrivalOverviewEndCamera_);
    ImGuiHelper::OnDrawInputField("arrivalIslandTitle_", arrivalIslandTitle_);
    ImGuiHelper::OnDrawInputField("arrivalIslandSubtitle_", arrivalIslandSubtitle_);
    ImGuiHelper::OnDrawInputField("arrivalTourShots_", arrivalTourShots_, [this]
    {
        if (ImGui::Button("Add Shot"))
            arrivalTourShots_.emplace_back();
    });
    ImGuiHelper::OnDrawInputField("arrivalCaptionPrefab_", arrivalCaptionPrefab_);
    ImGuiHelper::OnDrawInputField("heartsRoot_", heartsRoot_);
    ImGuiHelper::OnDrawInputField("floatingRoot_", floatingRoot_);
    ImGuiHelper::OnDrawInputField("endingCamera_", endingCamera_);
    ImGuiHelper::OnDrawInputField("greenHeartTrail_", greenHeartTrail_);
    ImGuiHelper::OnDrawInputField("lightHeartTrail_", lightHeartTrail_);
    ImGuiHelper::OnDrawInputField("fireHeartTrail_", fireHeartTrail_);
    ImGuiHelper::OnDrawInputField("heartBurst_", heartBurst_);
    ImGuiHelper::OnDrawInputField("heartMoundCenterPos_", heartMoundCenterPos_);
    ImGuiHelper::OnDrawInputField("endingDelay_secs_", endingDelay_secs_);
    ImGuiHelper::OnDrawInputField("heartRise_secs_", heartRise_secs_);
    ImGuiHelper::OnDrawInputField("heartFly_secs_", heartFly_secs_);
    ImGuiHelper::OnDrawInputField("heartStagger_secs_", heartStagger_secs_);
    ImGuiHelper::OnDrawInputField("endingHold_secs_", endingHold_secs_);
    ImGuiHelper::OnDrawInputField("heartRiseHeight_", heartRiseHeight_);
    ImGuiHelper::OnDrawInputField("heartFlyDistance_", heartFlyDistance_);
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Scene::DragonNestSceneContext, GameCore::Scene::SceneContextBase);
#pragma endregion
