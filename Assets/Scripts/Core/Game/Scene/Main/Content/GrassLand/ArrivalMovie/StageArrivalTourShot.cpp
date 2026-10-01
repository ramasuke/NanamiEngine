#include "StageArrivalTourShot.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Scene::GrassLand
{
    void StageArrivalTourShot::Init()
    {
        startCamera.Init();
        endCamera  .Init();
    }

    void StageArrivalTourShot::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("title",          title);
        LibCore::ImGuiHelper::OnDrawInputField("subtitle",       subtitle);
        LibCore::ImGuiHelper::OnDrawInputField("startCamera",    startCamera);
        LibCore::ImGuiHelper::OnDrawInputField("endCamera",      endCamera);
        LibCore::ImGuiHelper::OnDrawInputField("duration_msecs", duration_msecs);
    }
}
