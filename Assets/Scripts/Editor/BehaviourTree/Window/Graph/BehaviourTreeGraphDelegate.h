#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Engine/Module/Gui/Graph/Editor/GraphDelegateBase.h"
#include "../Node/Npc_BehaviourNodeBase.h"

namespace NanamiEngine::Module::Gui::Graph
{
    class GraphEditorHost;
}

namespace Editor::Npc::Behaviour
{
    /**
     * @brief BehaviourTree（敵 / 友好 NPC 共通）を ImGuizmo GraphEditor に見せるアダプタ。
     * @note  切り離したサブツリーは浮きノードとして残す
     */
    class BehaviourTreeGraphDelegate final : public NanamiEngine::Module::Gui::Graph::GraphDelegateBase
    {
    public:
        /**
         * @brief グラフを 1 フレーム描画する
         * @param detachedNodes 浮きノードの一覧（ツリーが保存する）。Create / Paste / 切り離しで増える
         * @param readOnly      実行中ツリーの表示用。選択と Inspector 表示のみ行い、編集はしない
         */
        void Draw(const std::shared_ptr<NodeBase>& entryNode,
                  std::vector<std::shared_ptr<NodeBase>>& detachedNodes,
                  NanamiEngine::Module::Gui::Graph::GraphEditorHost& host,
                  bool readOnly);

        // GraphEditor::Delegate
        bool AllowedLink(GraphEditor::NodeIndex from, GraphEditor::NodeIndex to) override;
        void MoveSelectedNodes(ImVec2 delta) override;
        void AddLink(GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex inputSlotIndex,
                     GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex outputSlotIndex) override;
        void DelLink(GraphEditor::LinkIndex linkIndex) override;
        void CustomDraw(ImDrawList* drawList, ImRect rectangle, GraphEditor::NodeIndex nodeIndex) override;
        const size_t GetTemplateCount() override;
        const GraphEditor::Template GetTemplate(GraphEditor::TemplateIndex index) override;
        const size_t GetNodeCount() override;
        const GraphEditor::Node GetNode(GraphEditor::NodeIndex index) override;
        const size_t GetLinkCount() override;
        const GraphEditor::Link GetLink(GraphEditor::LinkIndex index) override;
        void NodeDoubleClicked(GraphEditor::NodeIndex nodeIndex) override;

    protected:
        // GraphDelegateBase
        void Rebuild() override;
        [[nodiscard]] Guid NodeGuid(GraphEditor::NodeIndex nodeIndex) const override;
        [[nodiscard]] std::weak_ptr<NanamiEngine::Module::Object::IObject> InspectTarget(GraphEditor::NodeIndex nodeIndex) const override;
        void OnRightClickNode(GraphEditor::NodeIndex nodeIndex) override;
        void OnRightClickLink(GraphEditor::LinkIndex linkIndex) override;
        void DrawBackgroundMenu() override;
        void DrawToolbarItems() override;
        void DrawNodeMenu() override;
        void DrawLinkMenu() override;
        void DeleteSelection() override;

    private:
        static constexpr GraphEditor::NodeIndex INVALID_INDEX = static_cast<GraphEditor::NodeIndex>(-1);

        struct NodeEntry
        {
            std::shared_ptr<NodeBase> node;
            std::string               title;
            std::string               detail;
            ImVec2                    size;
            /** @brief リスト表示している子（畳まれた葉だけの Sequence） */
            std::vector<std::shared_ptr<NodeBase>> rows;
            GraphEditor::NodeIndex    parent      = INVALID_INDEX;
            std::size_t               childOrder  = 0;
            std::size_t               siblingCount = 0;
            bool                      isDetached  = false;
        };

        struct LinkEntry
        {
            GraphEditor::NodeIndex parent;
            GraphEditor::NodeIndex child;
        };

        /** @brief 直前に切り離した子。同じ親へ繋ぎ直したら元の順番（と重み）に戻す */
        struct LastDetached
        {
            std::weak_ptr<NodeBase> parent;
            std::weak_ptr<NodeBase> child;
            ChildSlot               slot;
        };

        /** @brief ノードの親子関係からリンク列と親情報を作り直す（ノード列と index は変えない） */
        void RebuildLinks();

        /** @brief child を今の親（または浮きノード一覧）から外して浮きノードにする */
        void Detach(const std::shared_ptr<NodeBase>& child);
        /** @brief 浮きノードの child を parent に繋ぐ。子を 1 つしか持てない親の既存の子は浮きノードへ移す */
        void Attach(const std::shared_ptr<NodeBase>& parent, const std::shared_ptr<NodeBase>& child);
        /** @brief ノードを消す。keepChildren なら子は浮きノードとして残し、false ならサブツリーごと消す */
        void DeleteNode(const std::shared_ptr<NodeBase>& node, bool keepChildren);
        void AddChildNode(const std::shared_ptr<NodeBase>& parent, const std::shared_ptr<NodeBase>& child);
        void AddDetachedNode(const std::shared_ptr<NodeBase>& node, const glm::vec2& position);

        /** @brief 左→右のツリーに並べ直す（深さ = X、兄弟 = 上から実行順） */
        void AutoLayout();
        /** @brief node をサブツリーごと (x, y) から並べ、次の兄弟を置ける y を返す */
        float LayoutSubtree(const std::shared_ptr<NodeBase>& node, float x, float y, std::unordered_set<const NodeBase*>& visited);
        [[nodiscard]] ImVec2 NodeSize(const NodeBase* node) const;
        /** @brief サブツリーの一番下の y（ノードの下端） */
        [[nodiscard]] float SubtreeBottom(const std::shared_ptr<NodeBase>& root) const;

        /** @brief 子が全部アクションの Sequence（既定で畳んでリスト表示する） */
        [[nodiscard]] static bool IsListSequence(const NodeBase& node);
        [[nodiscard]] bool IsFolded(const NodeBase& node) const;
        void ToggleFold(const NodeBase& node);
        /** @brief 畳まれたノードの子へは降りずに集める */
        void CollectVisible(const std::shared_ptr<NodeBase>& root,
                            std::vector<std::shared_ptr<NodeBase>>& out,
                            std::unordered_set<const NodeBase*>& visited) const;

        [[nodiscard]] GraphEditor::NodeIndex IndexOf(const NodeBase* node) const;
        [[nodiscard]] std::shared_ptr<NodeBase> ParentOf(const NodeBase* node) const;
        [[nodiscard]] bool IsEntry(const NodeBase* node) const { return node == entryNode_.get(); }

        std::shared_ptr<NodeBase>               entryNode_;
        std::vector<std::shared_ptr<NodeBase>>* detachedNodes_ = nullptr;

        std::vector<NodeEntry>                                        nodes_;
        std::unordered_map<const NodeBase*, GraphEditor::NodeIndex>   indexOf_;
        std::vector<LinkEntry>                                        links_;

        // NOTE: GraphEditor はドラッグ中ノード index を覚えるので、並び順は初めて見た順で固定する
        std::unordered_map<const NodeBase*, std::uint64_t> firstSeenOrder_;
        std::uint64_t                                      nextSeenOrder_ = 0;

        /** @brief 既定の開閉を反転したノード */
        std::unordered_set<Guid, GuidHash> toggled_;
        bool                               pendingLayout_ = false;

        LastDetached            lastDetached_;
        std::weak_ptr<NodeBase> menuNode_;
        std::weak_ptr<NodeBase> menuLinkChild_;
    };
}
