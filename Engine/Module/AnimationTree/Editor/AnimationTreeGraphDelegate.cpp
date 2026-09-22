#include "AnimationTreeGraphDelegate.h"

#include <algorithm>
#include <cstdio>
#include <ranges>

#include "imgui_internal.h"
#include "../LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../AnimationTree.h"
#include "../Node/ClipNode/AnimationClipNode.h"
#include "../../Gui/Graph/Editor/GraphEditorHost.h"

namespace NanamiEngine::Module::AnimationTree
{
    namespace
    {
        constexpr auto K_BACKGROUND_MENU = "##AnimationTreeBackgroundMenu";
        constexpr auto K_NODE_MENU       = "##AnimationTreeNodeMenu";
        constexpr auto K_LINK_MENU       = "##AnimationTreeLinkMenu";

        constexpr ImU32 K_ENTRY_HEADER_COLOR     = IM_COL32(56 , 150, 90 , 255);
        constexpr ImU32 K_ANY_STATE_HEADER_COLOR = IM_COL32(52 , 120, 200, 255);
        constexpr ImU32 K_CLIP_HEADER_COLOR      = IM_COL32(120, 90 , 170, 255);

        constexpr ImU32 K_SELECTED_LINK_COLOR    = IM_COL32(255, 210, 90 , 255);
        constexpr ImU32 K_BLENDING_LINK_COLOR    = IM_COL32(255, 150, 40 , 255);

        constexpr ImU32 K_DETAIL_TEXT_COLOR      = IM_COL32(200, 200, 210, 255);
        constexpr ImU32 K_PLAYING_BORDER_COLOR   = IM_COL32(255, 210, 60 , 255);
        constexpr ImU32 K_FADEOUT_BORDER_COLOR   = IM_COL32(160, 140, 220, 255);
        constexpr ImU32 K_PROGRESS_BG_COLOR      = IM_COL32(20 , 20 , 20 , 230);
        constexpr ImU32 K_PROGRESS_FILL_COLOR    = IM_COL32(80 , 200, 120, 255);
    }

    void AnimationTreeGraphDelegate::Draw(AnimationTree& tree, Gui::Graph::GraphEditorHost& host, const bool readOnly)
    {
        tree_     = &tree;
        host_     = &host;
        readOnly_ = readOnly;

        Rebuild();
        host.Draw(*this, readOnly);
        DrawContextMenus();

        if (!readOnly_ && host.IsFocused() && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
            DeleteSelection();
    }

    void AnimationTreeGraphDelegate::Rebuild()
    {
        nodes_.clear();
        names_.clear();
        indexOf_.clear();
        links_.clear();

        auto addNode = [this](const std::shared_ptr<IAnimationNode>& node)
        {
            if (!node)
                return;
            indexOf_[node.get()] = nodes_.size();
            nodes_.push_back(node);
            names_.push_back(node->GraphNodeName());
        };
        addNode(tree_->entryNode_);
        addNode(tree_->visualAnyStateNode_);
        for (const auto& node : tree_->nodes_ | std::views::values)
            addNode(node);

        for (const auto& path : tree_->fromNodeNodePaths_)
            TryAddLinkEntry(path, false);
        for (const auto& path : tree_->fromAnyStateNodeNodePaths_)
            TryAddLinkEntry(path, true);

        // 削除済みノードの選択を掃除する
        std::erase_if(selectedNodes_, [this](const Guid& guid)
        {
            return std::ranges::none_of(nodes_, [&](const auto& node) { return node->GetGuid() == guid; });
        });
    }

    void AnimationTreeGraphDelegate::TryAddLinkEntry(const std::shared_ptr<AnimationNodePath>& path, const bool isFromAnyState)
    {
        if (!path)
            return;

        // 旧形式（visualFromNodeGuid_ 無し）のファイルは visualFromNode_ が空なので、実際の遷移元で代用する
        std::shared_ptr<IAnimationNode> from = path->GetVisualFromNode();
        if (!from)
            from = isFromAnyState ? std::static_pointer_cast<IAnimationNode>(tree_->visualAnyStateNode_) : path->GetFromNode();

        const GraphEditor::NodeIndex fromIndex = IndexOf(from);
        const GraphEditor::NodeIndex toIndex   = IndexOf(path->GetTargetNode());
        if (fromIndex == static_cast<GraphEditor::NodeIndex>(-1) || toIndex == static_cast<GraphEditor::NodeIndex>(-1))
            return;

        links_.push_back(LinkEntry{ path, fromIndex, toIndex, isFromAnyState });
    }

    // GraphEditor は AllowedLink(遷移先, 遷移元) の順で呼ぶ
    bool AnimationTreeGraphDelegate::AllowedLink(const GraphEditor::NodeIndex from, const GraphEditor::NodeIndex to)
    {
        const GraphEditor::NodeIndex target = from;
        const GraphEditor::NodeIndex source = to;
        if (readOnly_ || target == source || KindOf(target) != TEMPLATE_CLIP)
            return false;

        return std::ranges::none_of(links_, [&](const LinkEntry& link) { return link.from == source && link.to == target; });
    }

    void AnimationTreeGraphDelegate::SelectNode(const GraphEditor::NodeIndex nodeIndex, const bool selected)
    {
        if (nodeIndex >= nodes_.size())
            return;

        const auto& node = nodes_[nodeIndex];
        if (!selected)
        {
            selectedNodes_.erase(node->GetGuid());
            return;
        }

        selectedNodes_.insert(node->GetGuid());
        selectedPath_.reset();
        Gui::Graph::ShowInInspector(node);
    }

    void AnimationTreeGraphDelegate::MoveSelectedNodes(const ImVec2 delta)
    {
        if (readOnly_)
            return;

        for (GraphEditor::NodeIndex i = 0; i < nodes_.size(); ++i)
        {
            if (IsSelected(i))
                nodes_[i]->SetPosition(nodes_[i]->Position() + glm::vec2(delta.x, delta.y));
        }
    }

    // GraphEditor の Link は input = 遷移元（出力スロット側）, output = 遷移先（入力スロット側）
    void AnimationTreeGraphDelegate::AddLink(const GraphEditor::NodeIndex inputNodeIndex, GraphEditor::SlotIndex,
                                             const GraphEditor::NodeIndex outputNodeIndex, GraphEditor::SlotIndex)
    {
        if (readOnly_ || inputNodeIndex >= nodes_.size() || outputNodeIndex >= nodes_.size())
            return;

        const auto& source = nodes_[inputNodeIndex];
        const auto& target = nodes_[outputNodeIndex];
        const bool isFromAnyState = source == tree_->visualAnyStateNode_;

        const auto path = std::make_shared<AnimationNodePath>();
        path->SetFromNodeForGraphEditorGui(source, source);
        path->SetTargetNode(target);
        (isFromAnyState ? tree_->fromAnyStateNodeNodePaths_ : tree_->fromNodeNodePaths_).push_back(path);

        // 同じ Show 呼び出しの中で GetLinkCount / GetLink が続くので、キャッシュにも即反映する
        links_.push_back(LinkEntry{ path, inputNodeIndex, outputNodeIndex, isFromAnyState });
    }

    void AnimationTreeGraphDelegate::DelLink(const GraphEditor::LinkIndex linkIndex)
    {
        if (readOnly_ || linkIndex >= links_.size())
            return;

        const auto path = links_[linkIndex].path;
        links_.erase(links_.begin() + static_cast<std::ptrdiff_t>(linkIndex));
        DeletePath(path);
    }

    void AnimationTreeGraphDelegate::CustomDraw(ImDrawList* drawList, const ImRect rectangle, const GraphEditor::NodeIndex nodeIndex)
    {
        if (nodeIndex >= nodes_.size())
            return;

        const auto& node       = nodes_[nodeIndex];
        const auto& options    = host_->Options();
        // CustomDraw には拡大率が渡らないので、本文の幅から逆算する
        // （本文 = ノード矩形 * 拡大率 から、拡大されない角丸ぶんを上下左右に削ったもの）
        const float zoom       = (rectangle.GetWidth() + options.mRounding * 2.0f) / NODE_SIZE.x;
        const float fontSize   = ImGui::GetFontSize() * 0.85f * zoom;
        const ImVec2 nodeMin   = rectangle.Min - ImVec2(options.mRounding, options.mHeaderHeight * zoom + options.mRounding);
        const ImVec2 nodeMax   = rectangle.Max + ImVec2(options.mRounding, options.mRounding);

        if (const std::string detail = node->GraphNodeDetail(); !detail.empty() && fontSize >= 6.0f)
        {
            drawList->PushClipRect(rectangle.Min, rectangle.Max, true);
            drawList->AddText(ImGui::GetFont(), fontSize, rectangle.Min, K_DETAIL_TEXT_COLOR, detail.c_str());
            drawList->PopClipRect();
        }

        // 実行中の再生状態。末尾が遷移先（メインで再生中）、それ以外はブレンドでフェードアウト中
        const auto& currentNodes = tree_->currentNodes_;
        const auto  it = std::ranges::find(currentNodes, node);
        if (it == currentNodes.end())
            return;

        const bool isPlaying = std::next(it) == currentNodes.end();
        const float outline  = 3.0f * zoom;
        drawList->AddRect(nodeMin - ImVec2(outline, outline), nodeMax + ImVec2(outline, outline),
                          isPlaying ? K_PLAYING_BORDER_COLOR : K_FADEOUT_BORDER_COLOR, options.mRounding * zoom, 0, outline);

        const auto* clip = dynamic_cast<const AnimationClipNode*>(node.get());
        if (!clip)
            return;

        const ClipProgress progress = clip->GetClipProgress();
        const ImVec2 barMin = ImVec2(nodeMin.x, nodeMax.y + 4.0f * zoom);
        const ImVec2 barMax = ImVec2(nodeMax.x, barMin.y + 6.0f * zoom);
        drawList->AddRectFilled(barMin, barMax, K_PROGRESS_BG_COLOR, 2.0f * zoom);
        drawList->AddRectFilled(barMin, ImVec2(barMin.x + (barMax.x - barMin.x) * progress.normalizedTime, barMax.y), K_PROGRESS_FILL_COLOR, 2.0f * zoom);

        if (fontSize >= 6.0f)
        {
            char label[64];
            snprintf(label, sizeof(label), "%.2f / %.2fs  blend %.2f", progress.duringSecs, progress.durationSecs, clip->GetBlendRate());
            drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(barMin.x, barMax.y + 2.0f * zoom), IM_COL32_WHITE, label);
        }
    }

    void AnimationTreeGraphDelegate::RightClick(const GraphEditor::NodeIndex nodeIndex, GraphEditor::SlotIndex, GraphEditor::SlotIndex)
    {
        menuGraphPosition_ = host_->ScreenToGraph(ImGui::GetIO().MousePos);
        if (nodeIndex < nodes_.size())
        {
            menuNode_    = nodes_[nodeIndex];
            pendingMenu_ = PendingMenu::Node;
        }
        else
        {
            pendingMenu_ = PendingMenu::Background;
        }
    }

    const size_t AnimationTreeGraphDelegate::GetTemplateCount()
    {
        return TEMPLATE_COUNT;
    }

    const GraphEditor::Template AnimationTreeGraphDelegate::GetTemplate(const GraphEditor::TemplateIndex index)
    {
        switch (index)
        {
        case TEMPLATE_ENTRY:     return Gui::Graph::MakeNodeTemplate(K_ENTRY_HEADER_COLOR    , 0, 1);
        case TEMPLATE_ANY_STATE: return Gui::Graph::MakeNodeTemplate(K_ANY_STATE_HEADER_COLOR, 0, 1);
        default:                 return Gui::Graph::MakeNodeTemplate(K_CLIP_HEADER_COLOR     , 1, 1);
        }
    }

    const size_t AnimationTreeGraphDelegate::GetNodeCount()
    {
        return nodes_.size();
    }

    const GraphEditor::Node AnimationTreeGraphDelegate::GetNode(const GraphEditor::NodeIndex index)
    {
        const glm::vec2 position = nodes_[index]->Position();
        const ImVec2    min(position.x, position.y);
        return GraphEditor::Node
        {
            names_[index].c_str(),
            KindOf(index),
            ImRect(min, min + NODE_SIZE),
            IsSelected(index)
        };
    }

    const size_t AnimationTreeGraphDelegate::GetLinkCount()
    {
        return links_.size();
    }

    const GraphEditor::Link AnimationTreeGraphDelegate::GetLink(const GraphEditor::LinkIndex index)
    {
        const LinkEntry& link = links_[index];
        return GraphEditor::Link{ link.from, 0, link.to, 0 };
    }

    void AnimationTreeGraphDelegate::LinkClicked(const GraphEditor::LinkIndex linkIndex)
    {
        if (linkIndex >= links_.size())
            return;

        ClearNodeSelection();
        selectedPath_ = links_[linkIndex].path;
        Gui::Graph::ShowInInspector(links_[linkIndex].path);
    }

    void AnimationTreeGraphDelegate::RightClickLink(const GraphEditor::LinkIndex linkIndex)
    {
        if (linkIndex >= links_.size())
            return;

        menuPath_    = links_[linkIndex].path;
        pendingMenu_ = PendingMenu::Link;
    }

    ImU32 AnimationTreeGraphDelegate::LinkColor(const GraphEditor::LinkIndex linkIndex, const ImU32 defaultColor)
    {
        if (linkIndex >= links_.size())
            return defaultColor;

        const auto& path = links_[linkIndex].path;
        if (path == selectedPath_.lock())
            return K_SELECTED_LINK_COLOR;

        // ブレンド中（再生中ノードが 2 つ）のみ、遷移中の NodePath を強調表示する
        if (tree_->currentNodes_.size() >= 2 && path.get() == tree_->currentNodePath_)
            return K_BLENDING_LINK_COLOR;

        return defaultColor;
    }

    void AnimationTreeGraphDelegate::DrawContextMenus()
    {
        switch (pendingMenu_)
        {
        case PendingMenu::Background: ImGui::OpenPopup(K_BACKGROUND_MENU); break;
        case PendingMenu::Node:       ImGui::OpenPopup(K_NODE_MENU);       break;
        case PendingMenu::Link:       ImGui::OpenPopup(K_LINK_MENU);       break;
        case PendingMenu::None:       break;
        }
        pendingMenu_ = PendingMenu::None;

        if (ImGui::BeginPopup(K_BACKGROUND_MENU))
        {
            if (readOnly_)
            {
                ImGui::TextDisabled("実行中のツリーは編集できません");
            }
            else if (ImGui::MenuItem("Add AnimationClipNode"))
            {
                const auto newNode = std::make_shared<AnimationClipNode>(glm::vec2(menuGraphPosition_.x, menuGraphPosition_.y));
                tree_->nodes_[newNode->GetGuid()] = newNode;
            }
            if (ImGui::MenuItem("Fit All", "F"))
                host_->RequestFit();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup(K_NODE_MENU))
        {
            const auto node = menuNode_.lock();
            if (node && ImGui::MenuItem("Show in Inspector"))
                Gui::Graph::ShowInInspector(node);

            // Entry / AnyState はツリーに必ず 1 つずつ必要なので消せない
            const bool isClip = node && dynamic_cast<AnimationClipNode*>(node.get()) != nullptr;
            if (ImGui::MenuItem("Delete Node", "Del", false, !readOnly_ && isClip))
                DeleteNode(node);
            if (ImGui::MenuItem("Delete Selected Nodes", nullptr, false, !readOnly_ && !selectedNodes_.empty()))
                DeleteSelection();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup(K_LINK_MENU))
        {
            const auto path = menuPath_.lock();
            if (path && ImGui::MenuItem("Show in Inspector"))
            {
                ClearNodeSelection();
                selectedPath_ = path;
                Gui::Graph::ShowInInspector(path);
            }
            if (ImGui::MenuItem("Delete Transition", nullptr, false, !readOnly_ && path != nullptr))
                DeletePath(path);
            ImGui::EndPopup();
        }
    }

    void AnimationTreeGraphDelegate::DeleteSelection()
    {
        if (const auto path = selectedPath_.lock())
        {
            DeletePath(path);
            return;
        }

        std::vector<std::shared_ptr<IAnimationNode>> targets;
        for (GraphEditor::NodeIndex i = 0; i < nodes_.size(); ++i)
        {
            if (IsSelected(i) && KindOf(i) == TEMPLATE_CLIP)
                targets.push_back(nodes_[i]);
        }
        for (const auto& node : targets)
            DeleteNode(node);
    }

    void AnimationTreeGraphDelegate::DeleteNode(const std::shared_ptr<IAnimationNode>& node)
    {
        if (!node || !dynamic_cast<AnimationClipNode*>(node.get()))
            return;

        const Guid guid = node->GetGuid();
        tree_->nodes_.erase(guid);
        selectedNodes_.erase(guid);

        // このノードに出入りする遷移も消す（参照先が消えた遷移はロード時に復元できないため）
        auto referencesNode = [&](const std::shared_ptr<AnimationNodePath>& path)
        {
            if (!path)
                return true;
            const auto from   = path->GetFromNode();
            const auto visual = path->GetVisualFromNode();
            const auto target = path->GetTargetNode();
            return !target || target == node || from == node || visual == node;
        };
        std::erase_if(tree_->fromNodeNodePaths_, referencesNode);
        std::erase_if(tree_->fromAnyStateNodeNodePaths_, referencesNode);
    }

    void AnimationTreeGraphDelegate::DeletePath(const std::shared_ptr<AnimationNodePath>& path)
    {
        if (!path)
            return;

        std::erase(tree_->fromNodeNodePaths_, path);
        std::erase(tree_->fromAnyStateNodeNodePaths_, path);
        if (selectedPath_.lock() == path)
            selectedPath_.reset();
    }

    void AnimationTreeGraphDelegate::ClearNodeSelection()
    {
        selectedNodes_.clear();
    }

    GraphEditor::NodeIndex AnimationTreeGraphDelegate::IndexOf(const std::shared_ptr<IAnimationNode>& node) const
    {
        const auto it = indexOf_.find(node.get());
        return (node && it != indexOf_.end()) ? it->second : static_cast<GraphEditor::NodeIndex>(-1);
    }

    AnimationTreeGraphDelegate::TemplateKind AnimationTreeGraphDelegate::KindOf(const GraphEditor::NodeIndex nodeIndex) const
    {
        const auto* node = nodes_[nodeIndex].get();
        if (node == tree_->entryNode_.get())
            return TEMPLATE_ENTRY;
        if (node == tree_->visualAnyStateNode_.get())
            return TEMPLATE_ANY_STATE;
        return TEMPLATE_CLIP;
    }

    bool AnimationTreeGraphDelegate::IsSelected(const GraphEditor::NodeIndex nodeIndex) const
    {
        return selectedNodes_.contains(nodes_[nodeIndex]->GetGuid());
    }
}
