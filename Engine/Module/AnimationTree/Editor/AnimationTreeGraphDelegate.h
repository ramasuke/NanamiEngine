#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GraphEditor.h"
#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::Gui::Graph
{
    class GraphEditorHost;
}

namespace NanamiEngine::Module::AnimationTree
{
    class AnimationTree;
    class AnimationNodePath;
    class IAnimationNode;

    /**
     * @brief AnimationTree を ImGuizmo GraphEditor に見せるアダプタ。
     * @note  毎フレーム Draw の先頭で AnimationTree からノード列・リンク列を作り直し、index ⇔ ノードを対応付ける。
     *        ノード位置は各ノードの position_ をそのまま使う（セーブ形式は変えない）。
     *        遷移（AnimationNodePath）はリンクとして表示し、出力スロット → 入力スロットのドラッグで追加、
     *        クリックで Inspector 表示、右クリック / Delete キーで削除する。
     */
    class NANAMI_API AnimationTreeGraphDelegate final : public GraphEditor::Delegate
    {
    public:
        /**
         * @brief グラフを 1 フレーム描画する（GraphEditor::Show + 右クリックメニュー + Delete キー）
         * @param readOnly 実行中ツリーの表示用。選択と Inspector 表示のみ行い、編集はしない
         */
        void Draw(AnimationTree& tree, Gui::Graph::GraphEditorHost& host, bool readOnly);

        // GraphEditor::Delegate
        bool AllowedLink(GraphEditor::NodeIndex from, GraphEditor::NodeIndex to) override;
        void SelectNode(GraphEditor::NodeIndex nodeIndex, bool selected) override;
        void MoveSelectedNodes(ImVec2 delta) override;
        void AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex inputSlotIndex,
                     GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex outputSlotIndex) override;
        void DelLink(GraphEditor::LinkIndex linkIndex) override;
        void CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex) override;
        void RightClick(GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex slotIndexInput, GraphEditor::SlotIndex slotIndexOutput) override;
        const size_t GetTemplateCount() override;
        const GraphEditor::Template GetTemplate(GraphEditor::TemplateIndex index) override;
        const size_t GetNodeCount() override;
        const GraphEditor::Node GetNode(GraphEditor::NodeIndex index) override;
        const size_t GetLinkCount() override;
        const GraphEditor::Link GetLink(GraphEditor::LinkIndex index) override;
        void LinkClicked(GraphEditor::LinkIndex linkIndex) override;
        void RightClickLink(GraphEditor::LinkIndex linkIndex) override;
        ImU32 LinkColor(GraphEditor::LinkIndex linkIndex, ImU32 defaultColor) override;

    private:
        enum TemplateKind : GraphEditor::TemplateIndex
        {
            TEMPLATE_ENTRY,
            TEMPLATE_ANY_STATE,
            TEMPLATE_CLIP,
            TEMPLATE_COUNT
        };

        enum class PendingMenu
        {
            None,
            Background,
            Node,
            Link
        };

        struct NANAMI_API LinkEntry
        {
            std::shared_ptr<AnimationNodePath> path;
            GraphEditor::NodeIndex             from;
            GraphEditor::NodeIndex             to;
            bool                               isFromAnyState;
        };

        void Rebuild();
        void TryAddLinkEntry(const std::shared_ptr<AnimationNodePath>& path, bool isFromAnyState);
        void DrawContextMenus();
        void DeleteSelection();
        void DeleteNode(const std::shared_ptr<IAnimationNode>& node);
        void DeletePath(const std::shared_ptr<AnimationNodePath>& path);
        void ClearNodeSelection();

        [[nodiscard]] GraphEditor::NodeIndex IndexOf(const std::shared_ptr<IAnimationNode>& node) const;
        [[nodiscard]] TemplateKind KindOf(GraphEditor::NodeIndex nodeIndex) const;
        [[nodiscard]] bool IsSelected(GraphEditor::NodeIndex nodeIndex) const;

        AnimationTree*                   tree_     = nullptr;
        Gui::Graph::GraphEditorHost*     host_     = nullptr;
        bool                             readOnly_ = false;

        std::vector<std::shared_ptr<IAnimationNode>>             nodes_;
        std::vector<std::string>                                 names_;
        std::unordered_map<const IAnimationNode*, GraphEditor::NodeIndex> indexOf_;
        std::vector<LinkEntry>                                   links_;

        std::unordered_set<Guid, GuidHash> selectedNodes_;
        std::weak_ptr<AnimationNodePath>   selectedPath_;

        PendingMenu                      pendingMenu_       = PendingMenu::None;
        ImVec2                           menuGraphPosition_ = ImVec2(0, 0);
        std::weak_ptr<IAnimationNode>    menuNode_;
        std::weak_ptr<AnimationNodePath> menuPath_;
    };
}
