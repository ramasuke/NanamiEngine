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
#include "../Node/Sequence/Npc_Behaviour_SequenceNode.h"

namespace Editor::Npc::Behaviour
{
    namespace
    {
        namespace Graph = NanamiEngine::Module::Gui::Graph;

        constexpr ImU32 K_DETAIL_TEXT_COLOR   = IM_COL32(200, 200, 210, 255);
        constexpr ImU32 K_ORDER_BADGE_COLOR   = IM_COL32(255, 255, 255, 200);
        constexpr ImU32 K_DETACHED_SHADE      = IM_COL32(20 , 20 , 24 , 120);
        constexpr ImU32 K_DETACHED_TEXT_COLOR = IM_COL32(170, 170, 180, 255);
        constexpr ImU32 K_ROW_HOVER_COLOR     = IM_COL32(255, 255, 255, 24);
        constexpr ImU32 K_ROW_STATUS_ALPHA    = 0x60;

        // NOTE: tools/bt/layout.py と同じ値にする
        constexpr float K_LAYOUT_GAP_X = 40.0f;
        constexpr float K_LAYOUT_GAP_Y = 16.0f;

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

        std::string TitleOf(const NodeBase& node)
        {
            std::string title = node.GraphNodeTitle();
            return title.empty() ? node.GraphNodeDetail() : title;
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
        CollectVisible(entryNode_, all, visited);
        std::erase(*detachedNodes_, nullptr);
        for (const auto& root : *detachedNodes_)
            CollectVisible(root, all, visited);

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
            // NOTE: 名前の無いアクションは型名を見出しにする
            if (entry.title.empty())
                std::swap(entry.title, entry.detail);

            if (IsFolded(*node) && IsListSequence(*node))
            {
                entry.rows = node->Children();
                entry.detail.clear();
                for (std::size_t i = 0; i < entry.rows.size(); ++i)
                    entry.detail += (i == 0 ? "" : "\n") + std::to_string(i + 1) + ". " + TitleOf(*entry.rows[i]);
            }
            else if (IsFolded(*node))
            {
                std::vector<std::shared_ptr<NodeBase>> subtree;
                std::unordered_set<const NodeBase*>    subtreeVisited;
                CollectSubtree(node, subtree, subtreeVisited);
                entry.title += " (+" + std::to_string(subtree.size() - 1) + ")";
            }
            nodes_.push_back(std::move(entry));
        }

        RebuildLinks();

        if (pendingLayout_)
        {
            pendingLayout_ = false;
            AutoLayout();
        }
    }

    bool BehaviourTreeGraphDelegate::IsListSequence(const NodeBase& node)
    {
        if (!dynamic_cast<const SequenceNode*>(&node))
            return false;

        const auto children = node.Children();
        return !children.empty() && std::ranges::all_of(children, [](const auto& child)
        {
            return child && child->MaxChildren() == 0;
        });
    }

    bool BehaviourTreeGraphDelegate::IsFolded(const NodeBase& node) const
    {
        if (node.Children().empty())
            return false;
        return IsListSequence(node) != toggled_.contains(node.GetGuid());
    }

    void BehaviourTreeGraphDelegate::ToggleFold(const NodeBase& node)
    {
        if (node.Children().empty())
            return;

        if (!toggled_.erase(node.GetGuid()))
            toggled_.insert(node.GetGuid());
        pendingLayout_ = !readOnly_;
    }

    void BehaviourTreeGraphDelegate::CollectVisible(const std::shared_ptr<NodeBase>& root,
                                                    std::vector<std::shared_ptr<NodeBase>>& out,
                                                    std::unordered_set<const NodeBase*>& visited) const
    {
        if (!root || !visited.insert(root.get()).second)
            return;

        out.push_back(root);
        if (IsFolded(*root))
            return;
        for (const auto& child : root->Children())
            CollectVisible(child, out, visited);
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

        for (auto& entry : nodes_)
            entry.size = MeasureNodeSize(entry.title, entry.detail, entry.siblingCount > 1);

        // NOTE: Entry まで辿り着けないノードは浮きノード（実行されない）
        for (auto& entry : nodes_)
        {
            const NodeEntry* root = &entry;
            for (std::size_t depth = 0; root->parent != INVALID_INDEX && depth < nodes_.size(); ++depth)
                root = &nodes_[root->parent];
            entry.isDetached = !IsEntry(root->node.get());
        }
    }

    // NOTE: from は子（入力スロット側）、to は親（出力スロット側）
    bool BehaviourTreeGraphDelegate::AllowedLink(const GraphEditor::NodeIndex from, const GraphEditor::NodeIndex to)
    {
        if (readOnly_ || from >= nodes_.size() || to >= nodes_.size() || from == to)
            return false;

        const auto& child  = nodes_[from].node;
        const auto& parent = nodes_[to].node;
        if (IsEntry(child.get()) || parent->MaxChildren() == 0)
            return false;

        // NOTE: 子のサブツリーに親が含まれていたら循環する
        std::vector<std::shared_ptr<NodeBase>> subtree;
        std::unordered_set<const NodeBase*>    visited;
        CollectSubtree(child, subtree, visited);
        return !visited.contains(parent.get());
    }

    void BehaviourTreeGraphDelegate::MoveSelectedNodes(const ImVec2 delta)
    {
        if (readOnly_)
            return;

        // NOTE: 選択ノードの子孫も相対位置を保って一緒に動かす
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

    // NOTE: inputNode は親（出力スロット側）、outputNode は子（入力スロット側）
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

        // NOTE: 同じフレーム内で続けてリンクが参照されるので、すぐ作り直す
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

        // NOTE: 子を 1 つしか持てない親なら、今の子を浮きノードにしてから繋ぐ
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

        if (IsFolded(*parent) && !IsListSequence(*parent))
            ToggleFold(*parent);

        // NOTE: 親の右、既存の子の下に置く（子を 1 つしか持てない親は置き換えるので親の右隣）
        glm::vec2 target = parent->PositionRef() + glm::vec2(NodeSize(parent.get()).x + K_LAYOUT_GAP_X, 0.0f);
        if (parent->MaxChildren() != 1 && !parent->Children().empty())
            target.y = SubtreeBottom(parent->Children().back()) + K_LAYOUT_GAP_Y;
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

    void BehaviourTreeGraphDelegate::AutoLayout()
    {
        if (!entryNode_)
            return;

        std::unordered_set<const NodeBase*> visited;
        const glm::vec2 origin = entryNode_->PositionRef();
        float y = LayoutSubtree(entryNode_, origin.x, origin.y, visited);
        for (const auto& root : *detachedNodes_)
        {
            if (root && !visited.contains(root.get()))
                y = LayoutSubtree(root, origin.x, y + K_LAYOUT_GAP_Y * 4.0f, visited);
        }
    }

    float BehaviourTreeGraphDelegate::LayoutSubtree(const std::shared_ptr<NodeBase>& node, const float x, const float y,
                                                    std::unordered_set<const NodeBase*>& visited)
    {
        if (!node || !visited.insert(node.get()).second)
            return y;

        const ImVec2 size = NodeSize(node.get());
        node->PositionRef() = glm::vec2(x, y);
        const float childX = x + size.x + K_LAYOUT_GAP_X;
        float       nextY  = y;
        for (const auto& child : node->Children())
            nextY = LayoutSubtree(child, childX, nextY, visited);

        // NOTE: 畳んだノードの子は展開したときの位置だけ決め、縦の場所は取らない
        if (IsFolded(*node))
            return y + size.y + K_LAYOUT_GAP_Y;
        return std::max(nextY, y + size.y + K_LAYOUT_GAP_Y);
    }

    ImVec2 BehaviourTreeGraphDelegate::NodeSize(const NodeBase* node) const
    {
        const GraphEditor::NodeIndex index = IndexOf(node);
        return index != INVALID_INDEX ? nodes_[index].size : NODE_SIZE;
    }

    float BehaviourTreeGraphDelegate::SubtreeBottom(const std::shared_ptr<NodeBase>& root) const
    {
        std::vector<std::shared_ptr<NodeBase>> subtree;
        std::unordered_set<const NodeBase*>    visited;
        CollectSubtree(root, subtree, visited);

        float bottom = -FLT_MAX;
        for (const auto& node : subtree)
            bottom = std::max(bottom, node->PositionRef().y + NodeSize(node.get()).y);
        return bottom;
    }

    void BehaviourTreeGraphDelegate::DrawToolbarItems()
    {
        if (ImGui::SmallButton("整列"))
        {
            AutoLayout();
            host_->RequestFit();
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("左から右へのツリーに並べ直す");
    }

    void BehaviourTreeGraphDelegate::CustomDraw(ImDrawList* drawList, const ImRect rectangle, const GraphEditor::NodeIndex nodeIndex)
    {
        if (nodeIndex >= nodes_.size())
            return;

        const auto&  entry    = nodes_[nodeIndex];
        const auto&  options  = host_->Options();
        const float  zoom     = Zoom();
        const float  fontSize = DetailFontSize();
        ImFont*      font     = Graph::FontForSize(fontSize);
        const ImRect frame    = NodeFrame(rectangle, zoom);
        const bool   canText  = fontSize >= 6.0f;

        if (!entry.rows.empty() && DetailVisible())
        {
            const ImVec2 mouse   = ImGui::GetIO().MousePos;
            const bool   hovered = rectangle.Contains(mouse) && ImGui::IsWindowHovered();
            for (std::size_t i = 0; i < entry.rows.size(); ++i)
            {
                const ImRect row(ImVec2(rectangle.Min.x, rectangle.Min.y + fontSize * static_cast<float>(i)),
                                 ImVec2(rectangle.Max.x, rectangle.Min.y + fontSize * static_cast<float>(i + 1)));
                if (const auto statusColor = DrawGraphEditorGuiHelper::RuntimeStatusColor(*entry.rows[i]))
                    drawList->AddRectFilled(row.Min, row.Max, (*statusColor & ~IM_COL32_A_MASK) | (K_ROW_STATUS_ALPHA << IM_COL32_A_SHIFT));
                if (hovered && row.Contains(mouse))
                {
                    drawList->AddRectFilled(row.Min, row.Max, K_ROW_HOVER_COLOR);
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        Graph::ShowInInspector(entry.rows[i]);
                }
            }
        }
        DrawNodeDetail(drawList, rectangle, entry.detail, K_DETAIL_TEXT_COLOR);

        // NOTE: 兄弟が複数あるときは実行順をヘッダー右端に出す
        if (entry.siblingCount > 1 && canText)
        {
            const std::string badge = "#" + std::to_string(entry.childOrder + 1);
            const ImVec2 size = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, badge.c_str());
            const ImVec2 position(frame.Max.x - size.x - 6.0f * zoom, frame.Min.y + (options.mHeaderHeight * zoom - size.y) * 0.5f);
            drawList->AddText(font, fontSize, position, K_ORDER_BADGE_COLOR, badge.c_str());
        }

        if (entry.isDetached)
        {
            drawList->AddRectFilled(frame.Min, frame.Max, K_DETACHED_SHADE, options.mRounding * zoom);
            if (canText)
                drawList->AddText(font, fontSize, ImVec2(frame.Min.x, frame.Max.y + 3.0f * zoom), K_DETACHED_TEXT_COLOR, "未接続（実行されません）");
        }

        // NOTE: 実行中ツリーのビューアでは、直近の Tick 結果で枠を色分けする
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

    // NOTE: テンプレートはノードごとに 1 つ（色と入出力の有無がノードの型で決まるため）
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
            ImRect(min, min + nodes_[index].size),
            IsSelected(index)
        };
    }

    void BehaviourTreeGraphDelegate::NodeDoubleClicked(const GraphEditor::NodeIndex nodeIndex)
    {
        if (nodeIndex < nodes_.size())
            ToggleFold(*nodes_[nodeIndex].node);
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
        if (!node->Children().empty() && ImGui::MenuItem(IsFolded(*node) ? "展開" : "折りたたむ", "ダブルクリック"))
            ToggleFold(*node);

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
        // NOTE: Entry はツリーに必ず 1 つ必要なので消せない
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
