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
    }

    void MainIslandSceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
        ImGuiHelper::OnDrawInputField("greenStone_", greenStone_);
        ImGuiHelper::OnDrawInputField("fountainIsland_", fountainIsland_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Scene::MainIslandSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::SceneContextBase, GameCore::Scene::MainIslandSceneContext);
#pragma endregion
