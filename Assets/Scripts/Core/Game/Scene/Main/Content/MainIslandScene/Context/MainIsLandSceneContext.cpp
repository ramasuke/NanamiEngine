#include "MainIsLandSceneContext.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Scene
{
    void MainIslandSceneContext::Init()
    {
        SceneContextBase::Init();
        bgm_.Init();
        greenStone_.Init();
        fountainIsland_.Init();
        lightStone_.Init();
        nestDepartureCamera_.Init();
        nestDepartureSound_.Init();
    }

    void MainIslandSceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
        ImGuiHelper::OnDrawInputField("greenStone_", greenStone_);
        ImGuiHelper::OnDrawInputField("fountainIsland_", fountainIsland_);
        ImGuiHelper::OnDrawInputField("lightStone_", lightStone_);
        ImGuiHelper::OnDrawInputField("nestDepartureCamera_", nestDepartureCamera_);
        ImGuiHelper::OnDrawInputField("nestDepartureSound_", nestDepartureSound_);
        ImGuiHelper::OnDrawInputField("nestDeparture_secs_", nestDeparture_secs_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Scene::MainIslandSceneContext, GameCore::Scene::SceneContextBase);
#pragma endregion
