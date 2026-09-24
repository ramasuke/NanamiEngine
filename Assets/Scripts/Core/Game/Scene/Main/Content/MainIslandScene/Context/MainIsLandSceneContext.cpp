#include "MainIsLandSceneContext.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Scene
{
    void MainIslandSceneContext::Init()
    {
        SceneContextBase::Init();
        bgm_.Init();
        greenStone_.Init();
        stoneCamera_.Init();
        stoneDockParticle_.Init();
        stoneFlightParticle_.Init();
        fountainIsland_.Init();
        fountainStairs_.Init();
        fountainCamera_.Init();
        fountainFocus_.Init();
    }

    void MainIslandSceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
        ImGuiHelper::OnDrawInputField("greenStone_", greenStone_);
        ImGuiHelper::OnDrawInputField("stoneCamera_", stoneCamera_);
        ImGuiHelper::OnDrawInputField("stoneDockParticle_", stoneDockParticle_);
        ImGuiHelper::OnDrawInputField("stoneFlightParticle_", stoneFlightParticle_);
        ImGuiHelper::OnDrawInputField("fountainIsland_", fountainIsland_);
        ImGuiHelper::OnDrawInputField("fountainStairs_", fountainStairs_);
        ImGuiHelper::OnDrawInputField("fountainCamera_", fountainCamera_);
        ImGuiHelper::OnDrawInputField("fountainFocus_", fountainFocus_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Scene::MainIslandSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::SceneContextBase, GameCore::Scene::MainIslandSceneContext);
#pragma endregion
