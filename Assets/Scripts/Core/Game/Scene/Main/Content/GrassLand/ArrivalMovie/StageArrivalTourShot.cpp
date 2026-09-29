#include "StageArrivalTourShot.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Scene::GrassLand
{
    void StageArrivalTourShot::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("title",          title);
        LibCore::ImGuiHelper::OnDrawInputField("subtitle",       subtitle);
        LibCore::ImGuiHelper::OnDrawInputField("cameraStart",    cameraStart);
        LibCore::ImGuiHelper::OnDrawInputField("cameraEnd",      cameraEnd);
        LibCore::ImGuiHelper::OnDrawInputField("lookAt",         lookAt);
        LibCore::ImGuiHelper::OnDrawInputField("duration_msecs", duration_msecs);
    }
}
