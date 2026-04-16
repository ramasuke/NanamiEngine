#include "Sub_SceneContextBase.h"

namespace GameCore::Scene::Sub
{
    void SceneContextBase::Initialize()
    {
        DoInitialize();
    }

    void SceneContextBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("sceneFile_", sceneFile_);
    }
}
