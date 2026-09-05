#include "DrawNodeHelper.h"

#include "imgui_internal.h"
#include "../../../../../Engine/Module/Exception/Engine_Module_Exception.h"
#include "../../../../../Engine/Module/Gui/Graph/GraphGui.h"
#include "../../../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../../../../../Engine/Module/Serialization/Engine_Module_Serialization.h"
#include "cereal/archives/portable_binary.hpp"
#include "Node/Npc_BehaviourNodeBase.h"
#include "Node/Npc_Behaviour_NodeFactory.h"
#include "../../Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionHeaders.h"
#include "../../Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionHeaders.h"

namespace
{
    std::stringstream s_copiedNodeBinary;
    bool s_hasCopiedNode = false;

    // 親ノードをドラッグ移動したとき、ぶら下がっている子孫ノードを同じ量だけ平行移動させる。
    // これにより Sequence 等のノードを動かすと、その配下のノード群が相対位置を保ったまま追従する。
    void TranslateSubtree(const std::shared_ptr<Editor::Npc::Behaviour::NodeBase>& node, const glm::vec2& delta)
    {
        if (!node)
            return;

        for (const auto& child : node->Children())
        {
            if (!child)
                continue;

            child->PositionRef() += delta;
            TranslateSubtree(child, delta);
        }
    }
}

void Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::DrawNode(
    const std::weak_ptr<NodeBase>& drawNodeObj,
    const ImVec2& offset,
    glm::vec2& positionRef,
    ImDrawList* drawList,
    const Gui::Graph::NodeOption& option,
    const bool addNodeContextMenu)
{
    const auto node = drawNodeObj.lock();
    if (!node)
        return;

    const glm::vec2 beforePosition = positionRef;

    Gui::Graph::DrawNode(offset, positionRef, drawList, drawNodeObj, option, node->GetGuid());

    // このノードがドラッグ移動されたら、その分だけ子孫ノードも追従させる。
    if (const glm::vec2 dragDelta = positionRef - beforePosition;
        dragDelta.x != 0.0f || dragDelta.y != 0.0f)
    {
        TranslateSubtree(node, dragDelta);
    }

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
        ImGui::OpenPopup(("NodeContextMenu##" + node->GetGuid().Value()).c_str());
    }

    // --- ポップアップ ---
    if (ImGui::BeginPopup(("NodeContextMenu##" + node->GetGuid().Value()).c_str()))
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
                if (const auto pastedNode = PasteNode())
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
                nextNode->PositionRef() = node->PositionRef() + glm::vec2(0.0f, 100.0f);
                node->SetConnectToNextNode(nextNode);
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

    // 2 回目以降の貼り付けでも先頭から読めるよう、読み取り位置を戻す
    s_copiedNodeBinary.clear();
    s_copiedNodeBinary.seekg(0);

    std::shared_ptr<NodeBase> newNode;
    try
    {
        NanamiEngine::Module::Serialization::LoadPortableBinary(s_copiedNodeBinary, "BehaviourTree clipboard", [&newNode](cereal::PortableBinaryInputArchive& archive)
        {
            archive(newNode);
        });
    }
    catch (const NanamiEngine::Module::Exception::SerializationException& exception)
    {
        NanamiEngine::Module::LogError("DrawNodeHelper: ノードの貼り付けに失敗しました: " + std::string(exception.what()));
        return nullptr;
    }

    if(newNode) newNode->ResetGuid();
    return newNode;
}

bool Editor::Npc::Behaviour::DrawGraphEditorGuiHelper::HasCopiedNode()
{
    return s_hasCopiedNode;
}
