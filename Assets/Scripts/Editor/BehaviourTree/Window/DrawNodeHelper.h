#pragma once
#include <memory>

#include "vec2.hpp"


namespace Editor::Npc::Behaviour
{
    class NodeBase;
}

namespace NanamiEngine::Module::Gui::Graph
{
    struct NodeOption;
    struct NodeVisualStyle;
}

struct ImVec2;
struct ImDrawList;

namespace Editor::Npc::Behaviour::DrawGraphEditorGuiHelper
{
    void DrawNode(
        const std::weak_ptr<NodeBase>& drawNodeObj,
        const ImVec2& offset,
        glm::vec2& positionRef,
        ImDrawList* drawList,
        const NanamiEngine::Module::Gui::Graph::NodeOption& option,
        bool addNodeContextMenu);
    void DrawNodePath(const ImVec2& offset, NodeBase& fromNode, NodeBase& toNode, ImDrawList* drawList);
    void CopyNode(const std::weak_ptr<NodeBase>& copyNode);
    std::shared_ptr<NodeBase> PasteNode();
    bool HasCopiedNode();

    /**
     * @note ノードが直近でTickされていれば、その結果（Success/Running/Failure/Abort）に応じて
     *       枠線の色を上書きしたNodeVisualStyleを返す。まだTickされていなければbaseStyleをそのまま返す。
     *       BehaviourTreeビューア（実行中ツリーの表示）でのみ意味を持ち、通常の編集画面ではbaseStyleのまま。
     */
    NanamiEngine::Module::Gui::Graph::NodeVisualStyle ApplyRuntimeStatusStyle(
        const NodeBase& node,
        const NanamiEngine::Module::Gui::Graph::NodeVisualStyle& baseStyle);
};
