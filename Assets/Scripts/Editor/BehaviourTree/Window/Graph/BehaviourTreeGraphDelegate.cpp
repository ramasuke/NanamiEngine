#include "BehaviourTreeGraphDelegate.h"

#include <algorithm>
#include <cfloat>
#include <ranges>
#include <string>
#include <unordered_set>

#include "imgui_internal.h"
#include "Engine/Module/Gui/Graph/Editor/GraphEditorHost.h"
#include "../DrawNodeHelper.h"
#include "../Node/Npc_Behaviour_NodeFactory.h"

namespace Editor::Npc::Behaviour
{
    namespace
    {
        namespace Graph = NanamiEngine::Module::Gui::Graph;

        constexpr ImU32 K_DETAIL_TEXT_COLOR   = IM_COL32(200, 200, 210, 255);
        constexpr ImU32 K_ORDER_BADGE_COLOR   = IM_COL32(255, 255, 255, 200);
        constexpr ImU32 K_DETACHED_SHADE      = IM_COL32(20 , 20 , 24 , 120);
        constexpr ImU32 K_DETACHED_TEXT_COLOR = IM_COL32(170, 170, 180, 255);

        /** @brief 子ノードを親の下に並べるときの間隔 */
        constexpr float K_CHILD_OFFSET_Y = 100.0f;
        constexpr float K_CHILD_SPACING_X = 20.0f;

        void CollectSubtree(const std::shared_ptr<NodeBase>& root,
                            std::vector<std::shared_ptr<NodeBase>>& out,
                            std::unordered_set<const NodeBase*>& visited)
        {
            if (!root || !visited.insert(root.get()).second)
                return;

            out.push_back(root);
            for (const auto& child : root->Children())
                CollectSubtree(child, out, visited);
        }

        void TranslateSubtree(const std::shared_ptr<NodeBase>& root, const glm::vec2& delta)
        {
            std::vector<std::shared_ptr<NodeBase>> subtree;
            std::unordered_set<const NodeBase*>    visited;
            CollectSubtree(root, subtree, visited);
            for (const auto& node : subtree)
                node->PositionRef() += delta;
        }

        std::vector<std::string> SortedCreatableNodeNames()
        {
            const auto creatableNodes = NodeFactory::Instance().CreatableNodes();
            std::vector<std::string> names;
            for (const auto& name : creatableNodes | std::views::keys)
                names.push_back(name);
            std::ranges::sort(names);
            return names;
        }
    }

    void BehaviourTreeGraphDelegate::Draw(const std::shared_ptr<NodeBase>& entryNode,
                                          std::vector<std::shared_ptr<NodeBase>>& detachedNodes,
                                          Graph::GraphEditorHost& host,
                                          const bool readOnly)
    {
        entryNode_     = entryNode;
        detachedNodes_ = &detachedNodes;
        DrawFrame(host, readOnly);
    }

    void BehaviourTreeGraphDelegate::Rebuild()
    {
        std::vector<std::shared_ptr<NodeBase>> all;
        std::unordered_set<const NodeBase*>    visited;
        CollectSubtree(entryNode_, all, visited);
        std::erase(*detachedNodes_, nullptr);
        for (const auto& root : *detachedNodes_)
            CollectSubtree(root, all, visited);

        for (const auto& node : all)
        {
            if (!firstSeenOrder_.contains(node.get()))
                firstSeenOrder_[node.get()] = nextSeenOrder_++;
        }
        std::erase_if(firstSeenOrder_, [&](const auto& pair) { return !visited.contains(pair.first); });
        std::ranges::sort(all, [this](const auto& lhs, const auto& rhs)
        {
            return firstSeenOrder_.at(lhs.get()) < firstSeenOrder_.at(rhs.get());
        });

        nodes_.clear();
        indexOf_.clear();
        for (const auto& node : all)
        {
            indexOf_[node.get()] = nodes_.size();

            NodeEntry entry;
            entry.node   = node;
            entry.title  = node->GraphNodeTitle();
            entry.detail = node->GraphNodeDetail();
            // 名前の無いアクションは型名を見出しにする
            if (entry.title.empty())
                std::swap(entry.title, entry.detail);
            nodes_.push_back(std::move(entry));
        }

        RebuildLinks();
    }

    void BehaviourTreeGraphDelegate::RebuildLinks()
    {
        links_.clear();
        for (auto& entry : nodes_)
        {
            entry.parent       = INVALID_INDEX;
            entry.childOrder   = 0;
            entry.siblingCount = 0;
            entry.isDetached   = false;
        }

        for (GraphEditor::NodeIndex i = 0; i < nodes_.size(); ++i)
        {
            const auto children = nodes_[i].node->Children();
            for (std::size_t order = 0; order < children.size(); ++order)
            {
                const GraphEditor::NodeIndex childIndex = IndexOf(children[order].get());
                if (childIndex == INVALID_INDEX)
                    continue;

                nodes_[childIndex].parent       = i;
                nodes_[childIndex].childOrder   = order;
                nodes_[childIndex].siblingCount = children.size();
                links_.push_back(LinkEntry{ i, childIndex });
            }
        }

        // Entry まで辿り着けないノードは浮きノード（実行されない）
        for (auto& entry : nodes_)
        {
            const NodeEntry* root = &entry;
            for (std::size_t depth = 0; root->parent != INVALID_INDEX && depth < nodes_.size(); ++depth)
                root = &nodes_[root->parent];
            entry.isDetached = !IsEntry(root->node.get());
        }
    }

    // GraphEditor は AllowedLink(子 = 入力スロット側, 親 = 出力スロット側) の順で呼ぶ
    bool BehaviourTreeGraphDelegate::AllowedLink(const GraphEditor::NodeIndex from, const GraphEditor::NodeIndex to)
    {
        if (readOnly_ || from >= nodes_.size() || to >= nodes_.size() || from == to)
            return false;

        const auto& child  = nodes_[from].node;
        const auto& parent = nodes_[to].node;
        if (IsEntry(child.get()) || parent->MaxChildren() == 0)
            return false;

        // 子のサブツリーに親が含まれていたら循環する
        std::vector<std::shared_ptr<NodeBase>> subtree;
        std::unordered_set<const NodeBase*>    visited;
        CollectSubtree(child, subtree, visited);
        return !visited.contains(parent.get());
    }

    void BehaviourTreeGraphDelegate::MoveSelectedNodes(const ImVec2 delta)
    {
        if (readOnly_)
            return;

        // 選択ノードの子孫も一緒に動かす（親をドラッグするとぶら下がっているノードが相対位置を保って付いてくる）
        std::vector<std::shared_ptr<NodeBase>> targets;
        std::unordered_set<const NodeBase*>    visited;
        for (GraphEditor::NodeIndex i = 0; i < nodes_.size(); ++i)
        {
            if (IsSelected(i))
                CollectSubtree(nodes_[i].node, targets, visited);
        }
        for (const auto& node : targets)
            node->PositionRef() += glm::vec2(delta.x, delta.y);
    }

    // GraphEditor の Link は input = 親（出力スロット側）, output = 子（入力スロット側）
    void BehaviourTreeGraphDelegate::AddLink(const GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex,
                                             const GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex)
    {
        if (readOnly_ || inputNodeIndex >= nodes_.size() || outputNodeIndex >= nodes_.size())
            return;

        Attach(nodes_[inputNodeIndex].node, nodes_[outputNodeIndex].node);
    }

    void BehaviourTreeGraphDelegate::DelLink(const GraphEditor::LinkIndex linkIndex)
    {
        if (readOnly_ || linkIndex >= links_.size())
            return;

        Detach(nodes_[links_[linkIndex].child].node);
    }

    void BehaviourTreeGraphDelegate::Detach(const std::shared_ptr<NodeBase>& child)
    {
        if (!child || IsEntry(child.get()))
            return;

        if (const auto parent = ParentOf(child.get()))
        {
            if (const auto slot = parent->RemoveChild(child.get()))
                lastDetached_ = LastDetached{ parent, child, *slot };
        }
        if (std::ranges::find(*detachedNodes_, child) == detachedNodes_->end())
            detachedNodes_->push_back(child);

        // 同じ Show 呼び出しの中で GetLinkCount / GetLink が続くので、リンクもすぐ作り直す
        RebuildLinks();
    }

    void BehaviourTreeGraphDelegate::Attach(const std::shared_ptr<NodeBase>& parent, const std::shared_ptr<NodeBase>& child)
    {
        if (!parent || !child || IsEntry(child.get()) || parent->MaxChildren() == 0)
            return;

        const LastDetached last = lastDetached_;
        if (const auto oldParent = ParentOf(child.get()))
        {
            if (oldParent == parent)
                return;
            Detach(child);
        }

        // 子を 1 つしか持てない親なら、今の子を浮きノードにしてから繋ぐ
        if (parent->Children().size() >= parent->MaxChildren())
        {
            for (const auto& existing : parent->Children())
                Detach(existing);
        }

        std::erase(*detachedNodes_, child);
        if (last.parent.lock() == parent && last.child.lock() == child)
            parent->InsertChild(child, last.slot);
        else
            parent->SetConnectToNextNode(child);
        lastDetached_ = {};

        RebuildLinks();
    }

    void BehaviourTreeGraphDelegate::DeleteNode(const std::shared_ptr<NodeBase>& node, const bool keepChildren)
    {
        if (!node || IsEntry(node.get()))
            return;

        if (keepChildren)
        {
            for (const auto& child : node->Children())
            {
                if (child && node->RemoveChild(child.get()))
                    detachedNodes_->push_back(child);
            }
        }

        if (const auto parent = ParentOf(node.get()))
            parent->RemoveChild(node.get());
        std::erase(*detachedNodes_, node);
        selectedNodes_.erase(node->GetGuid());

        RebuildLinks();
    }

    void BehaviourTreeGraphDelegate::AddChildNode(const std::shared_ptr<NodeBase>& parent, const std::shared_ptr<NodeBase>& child)
    {
        if (!parent || !child)
            return;

        // 既存の子の右隣に置く（子を 1 つしか持てない親は置き換えるので真下）
        const std::size_t column = parent->MaxChildren() == 1 ? 0 : parent->Children().size();
        const glm::vec2 target = parent->PositionRef() + glm::vec2(static_cast<float>(column) * (NODE_SIZE.x + K_CHILD_SPACING_X), K_CHILD_OFFSET_Y);
        TranslateSubtree(child, target - child->PositionRef());

        detachedNodes_->push_back(child);
        Attach(parent, child);
    }

    void BehaviourTreeGraphDelegate::AddDetachedNode(const std::shared_ptr<NodeBase>& node, const glm::vec2& position)
    {
        if (!node)
            return;

        TranslateSubtree(node, position - node->PositionRef());
        detachedNodes_->push_back(node);
    }

    void BehaviourTreeGraphDelegate::CustomDraw(ImDrawList* drawList, const ImRect rectangle, const GraphEditor::NodeIndex nodeIndex)
    {
        if (nodeIndex >= nodes_.size())
            return;

        const auto&  entry    = nodes_[nodeIndex];
        const auto&  options  = host_->Options();
        const float  zoom     = ZoomOf(rectangle, NODE_SIZE.x);
        const float  fontSize = ImGui::GetFontSize() * 0.85f * zoom;
        const ImRect frame    = NodeFrame(rectangle, zoom);
        const bool   canText  = fontSize >= 6.0f;

        if (!entry.detail.empty() && canText)
        {
            drawList->PushClipRect(rectangle.Min, rectangle.Max, true);
            drawList->AddText(ImGui::GetFont(), fontSize, rectangle.Min, K_DETAIL_TEXT_COLOR, entry.detail.c_str());
            drawList->PopClipRect();
        }

        // 兄弟が複数あるときは実行順をヘッダー右端に出す
        if (entry.siblingCount > 1 && canText)
        {
            const std::string badge = "#" + std::to_string(entry.childOrder + 1);
            const ImVec2 size = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, badge.c_str());
            const ImVec2 position(frame.Max.x - size.x - 6.0f * zoom, frame.Min.y + (options.mHeaderHeight * zoom - size.y) * 0.5f);
            drawList->AddText(ImGui::GetFont(), fontSize, position, K_ORDER_BADGE_COLOR, badge.c_str());
        }

        if (entry.isDetached)
        {
            drawList->AddRectFilled(frame.Min, frame.Max, K_DETACHED_SHADE, options.mRounding * zoom);
            if (canText)
                drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(frame.Min.x, frame.Max.y + 3.0f * zoom), K_DETACHED_TEXT_COLOR, "未接続（実行されません）");
        }

        // 実行中ツリーのビューアでは、直近の Tick 結果で枠を色分けする
        if (const auto statusColor = DrawGraphEditorGuiHelper::RuntimeStatusColor(*entry.node))
        {
            const float outline = 3.0f * zoom;
            drawList->AddRect(frame.Min - ImVec2(outline, outline), frame.Max + ImVec2(outline, outline),
                              *statusColor, options.mRounding * zoom, 0, outline);
        }
    }

    const size_t BehaviourTreeGraphDelegate::GetTemplateCount()
    {
        return nodes_.size();
    }

    // テンプレートはノードごとに 1 つ（色と入出力の有無がノードの型で決まるため）
    const GraphEditor::Template BehaviourTreeGraphDelegate::GetTemplate(const GraphEditor::TemplateIndex index)
    {
        if (index >= nodes_.size())
            return Graph::MakeNodeTemplate(IM_COL32(90, 90, 90, 255), 1, 0);

        const auto& node = nodes_[index].node;
        return Graph::MakeNodeTemplate(node->GraphHeaderColor(),
                                       IsEntry(node.get()) ? 0 : 1,
                                       node->MaxChildren() > 0 ? 1 : 0);
    }

    const size_t BehaviourTreeGraphDelegate::GetNodeCount()
    {
        return nodes_.size();
    }

    const GraphEditor::Node BehaviourTreeGraphDelegate::GetNode(const GraphEditor::NodeIndex index)
    {
        const glm::vec2 position = nodes_[index].node->PositionRef();
        const ImVec2    min(position.x, position.y);
        return GraphEditor::Node
        {
            nodes_[index].title.c_str(),
            index,
            ImRect(min, min + NODE_SIZE),
            IsSelected(index)
        };
    }

    const size_t BehaviourTreeGraphDelegate::GetLinkCount()
    {
        return links_.size();
    }

    const GraphEditor::Link BehaviourTreeGraphDelegate::GetLink(const GraphEditor::LinkIndex index)
    {
        const LinkEntry& link = links_[index];
        return GraphEditor::Link{ link.parent, 0, link.child, 0 };
    }

    Guid BehaviourTreeGraphDelegate::NodeGuid(const GraphEditor::NodeIndex nodeIndex) const
    {
        return nodes_[nodeIndex].node->GetGuid();
    }

    std::weak_ptr<NanamiEngine::Module::Object::IObject> BehaviourTreeGraphDelegate::InspectTarget(const GraphEditor::NodeIndex nodeIndex) const
    {
        return nodes_[nodeIndex].node;
    }

    void BehaviourTreeGraphDelegate::OnRightClickNode(const GraphEditor::NodeIndex nodeIndex)
    {
        menuNode_ = nodes_[nodeIndex].node;
    }

    void BehaviourTreeGraphDelegate::OnRightClickLink(const GraphEditor::LinkIndex linkIndex)
    {
        menuLinkChild_ = nodes_[links_[linkIndex].child].node;
    }

    void BehaviourTreeGraphDelegate::DrawBackgroundMenu()
    {
        if (readOnly_)
        {
            ImGui::TextDisabled("実行中のツリーは編集できません");
        }
        else
        {
            const glm::vec2 position(menuGraphPosition_.x, menuGraphPosition_.y);
            if (ImGui::BeginMenu("Create Node"))
            {
                for (const auto& name : SortedCreatableNodeNames())
                {
                    if (ImGui::MenuItem(name.c_str()))
                        AddDetachedNode(NodeFactory::Instance().Create(name), position);
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Paste", nullptr, false, DrawGraphEditorGuiHelper::HasCopiedNode()))
                AddDetachedNode(DrawGraphEditorGuiHelper::PasteNode(), position);
        }
        DrawFitAllMenuItem();
    }

    void BehaviourTreeGraphDelegate::DrawNodeMenu()
    {
        const auto node = menuNode_.lock();
        if (!node)
            return;

        if (ImGui::MenuItem("Show in Inspector"))
            Graph::ShowInInspector(node);
        if (ImGui::MenuItem("Copy"))
            DrawGraphEditorGuiHelper::CopyNode(node);

        if (readOnly_)
            return;

        ImGui::Separator();
        if (node->MaxChildren() > 0)
        {
            if (ImGui::BeginMenu("Create Child"))
            {
                for (const auto& name : SortedCreatableNodeNames())
                {
                    if (ImGui::MenuItem(name.c_str()))
                        AddChildNode(node, NodeFactory::Instance().Create(name));
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Paste as Child", nullptr, false, DrawGraphEditorGuiHelper::HasCopiedNode()))
                AddChildNode(node, DrawGraphEditorGuiHelper::PasteNode());
        }
        node->DrawGraphContextMenuItems();

        ImGui::Separator();
        const bool isEntry = IsEntry(node.get());
        if (ImGui::MenuItem("Disconnect from Parent", nullptr, false, ParentOf(node.get()) != nullptr))
            Detach(node);
        // Entry はツリーに必ず 1 つ必要なので消せない
        if (ImGui::MenuItem("Delete Node", "Del", false, !isEntry))
            DeleteNode(node, true);
        if (ImGui::MenuItem("Delete Subtree", nullptr, false, !isEntry))
            DeleteNode(node, false);
        if (ImGui::MenuItem("Delete Selected Nodes", nullptr, false, !selectedNodes_.empty()))
            DeleteSelection();
    }

    void BehaviourTreeGraphDelegate::DrawLinkMenu()
    {
        const auto child = menuLinkChild_.lock();
        if (child && ImGui::MenuItem("Show Child in Inspector"))
            Graph::ShowInInspector(child);
        if (ImGui::MenuItem("Disconnect", nullptr, false, !readOnly_ && child != nullptr))
            Detach(child);
    }

    void BehaviourTreeGraphDelegate::DeleteSelection()
    {
        std::vector<std::shared_ptr<NodeBase>> targets;
        for (GraphEditor::NodeIndex i = 0; i < nodes_.size(); ++i)
        {
            if (IsSelected(i) && !IsEntry(nodes_[i].node.get()))
                targets.push_back(nodes_[i].node);
        }
        for (const auto& node : targets)
            DeleteNode(node, true);
    }

    GraphEditor::NodeIndex BehaviourTreeGraphDelegate::IndexOf(const NodeBase* node) const
    {
        const auto it = indexOf_.find(node);
        return it != indexOf_.end() ? it->second : INVALID_INDEX;
    }

    std::shared_ptr<NodeBase> BehaviourTreeGraphDelegate::ParentOf(const NodeBase* node) const
    {
        const GraphEditor::NodeIndex index = IndexOf(node);
        if (index == INVALID_INDEX || nodes_[index].parent == INVALID_INDEX)
            return nullptr;
        return nodes_[nodes_[index].parent].node;
    }
}
