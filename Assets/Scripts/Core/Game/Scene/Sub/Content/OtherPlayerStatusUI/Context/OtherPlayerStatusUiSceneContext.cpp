#include "OtherPlayerStatusUiSceneContext.h"

namespace GameCore::Scene::Sub
{
    void OtherPlayerStatusUiSceneContext::DoInitialize()
    {
        ui_.Init();
    }

    void OtherPlayerStatusUiSceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("ui_", ui_);
    }
}
