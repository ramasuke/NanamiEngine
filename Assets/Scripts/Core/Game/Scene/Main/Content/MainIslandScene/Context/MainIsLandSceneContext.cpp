#include "MainIsLandSceneContext.h"

namespace GameCore::Scene
{
    void MainIslandSceneContext::Init()
    {
        summonPlayerAvatarPrefab_.Init();
    }

    void MainIslandSceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("summonPlayerAvatarPrefab_", summonPlayerAvatarPrefab_);
    }
}
