#include "GraphGui.h"

#include "../../../Core/Application/ApplicationBase.h"
#include "../../../Core/Application/Window/Popup/Group/PopupWindowGroup.h"
#include "../../../Core/Application/Window/Popup/Inspector/InspectorWindow.h"
#include "../LibCore/ImGui/Helper/ImGuiHelper.h"

void NanamiEngine::Module::Gui::Graph::DrawGrid(ImDrawList* drawList,
                                                const ImVec2& offset,
                                                const ImVec2& windowSize,
                                                const float gridStep,
                                                const ImU32 color)
{
    for (float x = 0.0f; x < windowSize.x; x += gridStep)
    {
        drawList->AddLine(
            offset + ImVec2(x, 0.0f),
            offset + ImVec2(x, windowSize.y),
            color
        );
    }

    for (float y = 0.0f; y < windowSize.y; y += gridStep)
    {
        drawList->AddLine(
            offset + ImVec2(0.0f, y),
            offset + ImVec2(windowSize.x, y),
            color
        );
    }
}

NanamiEngine::Module::Gui::Graph::NodeDrawResult NanamiEngine::Module::Gui::Graph::DrawNode(
    const ImVec2 offset,
    glm::vec2& nodePosition,
    ImDrawList* drawList,
    const std::weak_ptr<Object::IObject>& drawObject,
    const NodeOption& option,
    const Guid& nodeGuid)
{
    ImGui::PushID(nodeGuid.Value().c_str());
    
    const NodeVisualStyle& style    = option.Style();
    const ImVec2 nodeSize           = option.Size();
    const ImVec2 nodeScreenPosition = offset + ImVec2(nodePosition.x, nodePosition.y);

    // ノード背景の描画
    drawList->AddRectFilled(
        nodeScreenPosition,
        nodeScreenPosition + nodeSize,
        style.BackgroundColor(),
        6.0f);
    drawList->AddRect(
        nodeScreenPosition,
        nodeScreenPosition + nodeSize,
        style.BorderColor(),
        6.0f);
    drawList->AddText(nodeScreenPosition + ImVec2(10,10), IM_COL32_WHITE, option.Name().c_str());

    ImGui::SetCursorScreenPos(nodeScreenPosition);
    // ノード本体の描画
    ImGui::InvisibleButton("node_body", nodeSize);
    //Dragging時のノード移動
    if (ImGui::IsItemActive())
    {
        nodePosition += glm::vec2(
            ImGui::GetIO().MouseDelta.x,
            ImGui::GetIO().MouseDelta.y
        );
    }
    
    // Inspector連携
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        if (auto object = drawObject.lock())
        {
            for (auto* inspector : Core::Application::ApplicationBase::PopupWindows().Catch<Core::PopupWindow::InspectorWindow>())
            {
                inspector->TryAddDisplayObject(drawObject);
            }
        }
    }

    bool isReleasedInputPort = false;
    if (option.HasInput())
    {
        const ImVec2 inputPortPosition = nodeScreenPosition + ImVec2(0.0f, nodeSize.y * 0.5f);
        isReleasedInputPort = DrawInputPort(drawList, inputPortPosition, style.PortColor());
    }

    bool isConnectedOutputPort = false;
    if (option.HasOutput())
    {
        const ImVec2 outputPortPosition = nodeScreenPosition + ImVec2(nodeSize.x, nodeSize.y * 0.5f);
        isConnectedOutputPort = DrawOutputPort(drawList, outputPortPosition, style.PortColor());
    }

    ImGui::PopID();
    return NodeDrawResult(isConnectedOutputPort, isReleasedInputPort);
}

bool NanamiEngine::Module::Gui::Graph::DrawInputPort(ImDrawList* drawList, const ImVec2& position, const ImU32 color)
{
    drawList->AddCircleFilled(position, 5.0f, color);
    ImGui::SetCursorScreenPos(position - ImVec2(5.0f, 5.0f));
    ImGui::InvisibleButton("input_port", ImVec2(10.0f, 10.0f));

    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        return true;
    }

    return false;
}

bool NanamiEngine::Module::Gui::Graph::DrawOutputPort(ImDrawList* drawList, const ImVec2& position, const ImU32 color)
{
    drawList->AddCircleFilled(position, 5.0f, color);
    
    ImGui::SetCursorScreenPos(position - ImVec2(5.0f, 5.0f));
    ImGui::InvisibleButton("output_port", ImVec2(10.0f, 10.0f));

    return ImGui::IsItemActive();
}
