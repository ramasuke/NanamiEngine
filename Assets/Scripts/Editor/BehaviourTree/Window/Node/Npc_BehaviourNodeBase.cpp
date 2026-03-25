#include "Npc_BehaviourNodeBase.h"

#include "ImGuiHelper.h"

namespace Editor::Npc::Behaviour
{
    const auto NODE_SIZE = ImVec2(120, 60);

    void NodeBase::ResetGuid()
    {
        guid_ = Guid();
    }

    void NodeBase::OnDrawGui()
    {
        ImGui::Text(("guid_: " + guid_.Value()).c_str());
        DoOnDrawGui();
    }
}
