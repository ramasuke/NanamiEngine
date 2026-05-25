#include "MainIsLandSceneContext.h"

namespace GameCore::Scene
{
    void MainIslandSceneContext::Init()
    {
        SceneContextBase::Init();
        bgm_.Init();
    }

    void MainIslandSceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("bgm_", bgm_);
    }
}
