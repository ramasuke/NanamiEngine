#include "DrawNodeHelper.h"

#include "imgui_internal.h"
#include "../../../../../Engine/Module/Gui/Graph/GraphGui.h"
#include "cereal/archives/portable_binary.hpp"
#include "Node/Npc_BehaviourNodeBase.h"
#include "Node/Npc_Behaviour_NodeFactory.h"
#include "../../Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionHeaders.h"
#include "../../Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionHeaders.h"

namespace
{
    std::stringstream s_copiedNodeBinary;
    bool s_hasCopiedNode = false;
}

void Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::DrawNode(
    const std::weak_ptr<NodeBase>& drawNodeObj,
    const ImVec2& offset,
    glm::vec2& positionRef,
    ImDrawList* drawList,
    const Gui::Graph::NodeOption& option,
    const bool addNodeContextMenu)
{
    if (const auto node = drawNodeObj.lock(); !node)
        return;

    Gui::Graph::DrawNode(offset, positionRef, drawList, drawNodeObj, option, drawNodeObj.lock()->GetGuid());

    if (!addNodeContextMenu)
        return;
    
    //nodeの判定位置
    const ImVec2 nodeSize = option.Size();
    const auto position = ImVec2(positionRef.x, positionRef.y);
    const ImRect nodeRect(offset + position, offset + position + nodeSize);

    // マウスがノード上にあるか
    const bool hovered = ImGui::IsMouseHoveringRect(nodeRect.Min, nodeRect.Max);
    // 右クリックでポップアップを開く
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup(("NodeContextMenu##" + drawNodeObj.lock()->GetGuid().Value()).c_str());
    }

    // --- ポップアップ ---
    if (ImGui::BeginPopup(("NodeContextMenu##" + drawNodeObj.lock()->GetGuid().Value()).c_str()))
    {
        // Copy
        if (ImGui::MenuItem("Copy"))
        {
            CopyNode(drawNodeObj);
            ImGui::CloseCurrentPopup();
        }

        // Paste（コピーがある場合のみ有効）
        if (HasCopiedNode())
        {
            if (ImGui::MenuItem("Paste"))
            {
                const auto pastedNode = PasteNode();
                if (const auto node = drawNodeObj.lock(); node && pastedNode)
                {
                    node->SetConnectToNextNode(pastedNode);
                    pastedNode->PositionRef() = node->PositionRef() + glm::vec2(0.0f, 100.0f);
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Create Node");
        ImGui::Separator();

        const auto& creatableNodes = NodeFactory::Instance().CreatableNodes();
        for (const auto& [typeName, createFunc] : creatableNodes)
        {
            if (ImGui::MenuItem(typeName.c_str()))
            {
                const auto nextNode = createFunc();
                if (auto node = drawNodeObj.lock())
                {
                    nextNode->PositionRef() = node->PositionRef() + glm::vec2(0.0f, 100.0f);
                    node->SetConnectToNextNode(nextNode);
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

void Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::DrawNodePath(
    const ImVec2& offset,
    NodeBase& fromNode,
    NodeBase& toNode,
    ImDrawList* drawList)
{
    const auto fromNodePosition = ImVec2(fromNode.PositionRef().x, fromNode.PositionRef().y);
    const auto toNodePosition   = ImVec2(toNode  .PositionRef().x, toNode  .PositionRef().y); 
    drawList->AddLine(offset + fromNodePosition, offset + toNodePosition, IM_COL32(255, 255, 100, 255), 3.0f);
}

void Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::CopyNode(const std::weak_ptr<NodeBase>& copyNode)
{
    const auto node = copyNode.lock();
    if (!node) return;

    s_copiedNodeBinary.str({});
    s_copiedNodeBinary.clear();

    {
        cereal::PortableBinaryOutputArchive archive(s_copiedNodeBinary);
        archive(node);
    }

    s_hasCopiedNode = true;
}

std::shared_ptr<Editor::Npc::Behaviour::NodeBase> Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::PasteNode()
{
    if (!s_hasCopiedNode)
        return nullptr;

    std::shared_ptr<NodeBase> newNode;
    {
        cereal::PortableBinaryInputArchive archive(s_copiedNodeBinary);
        archive(newNode);
    }

    if(newNode) newNode->ResetGuid();
    return newNode;
}

bool Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::HasCopiedNode()
{
    return s_hasCopiedNode;
}
