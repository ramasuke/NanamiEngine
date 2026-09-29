#include "Navigation_NavigationTargetEntry.h"

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::Navigation
{
    bool NavigationTargetEntry::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("id_", id_);
        LibCore::ImGuiHelper::OnDrawInputField("object_", object_);
        LibCore::ImGuiHelper::OnDrawInputField("markerHeight_", markerHeight_);
        return ImGui::SmallButton("Remove");
    }
}
