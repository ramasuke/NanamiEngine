#include "AnimationTree.h"

#include <fstream>
#include <ranges>

#include "imgui_internal.h"
#include "../../Core/Application/Window/Main/Animator/AnimatorWindow.h"
#include "../../Core/Application/Window/Popup/Group/PopupWindowGroup.h"
#include "../../Core/Application/Window/Popup/Inspector/InspectorWindow.h"
#include "../Gui/Graph/GraphGui.h"
#include "cereal/archives/json.hpp"
#include "Node/ClipNode/AnimationClipNode.h"
#include "Node/EntryNode/AnimatorEntryNode.h"

AnimationTree::AnimationTree::AnimationTree(std::string filePath)
    : filePath_(std::move(filePath))
{
    std::ifstream ifStream(filePath_);
    if (!ifStream.is_open())
        return;

    cereal::JSONInputArchive archive(ifStream);
    archive(cereal::make_nvp("additionParameters_", additionConditionParameters_));
    archive(cereal::make_nvp("entryNode", entryNode_));
    archive(cereal::make_nvp("visualAnyStateNode", visualAnyStateNode_));

    std::size_t count = 0;
    archive(cereal::make_nvp("nodesCount", count));

    for (std::size_t i = 0; i < count; ++i)
    {
        std::shared_ptr<IAnimationNode> animationNode;
        archive(cereal::make_nvp("nodes_" + std::to_string(i), animationNode));
        if (animationNode)
        {
            nodes_[animationNode->GetGuid()] = animationNode;
        }
    }

    std::size_t pathCount = 0;
    archive(cereal::make_nvp("fromNodeNodePathCount", pathCount));

    for (std::size_t i = 0; i < pathCount; ++i)
    {
        std::shared_ptr<AnimationNodePath> path;
        archive(cereal::make_nvp("fromNodeNodePath_" + std::to_string(i), path));
        if (path)
        {
            fromNodeNodePaths_.push_back(path);
        }
    }

    std::size_t anyPathCount = 0;
    archive(cereal::make_nvp("fromAnyStateNodeNodePathCount", anyPathCount));

    for (std::size_t i = 0; i < anyPathCount; ++i)
    {
        std::shared_ptr<AnimationNodePath> path;
        archive(cereal::make_nvp("fromAnyStateNodeNodePath_" + std::to_string(i), path));
        if (path)
            fromAnyStateNodeNodePaths_.push_back(path);
    }

    for (const auto& path : AllNodePaths())
    {
        path->InitNodePath(additionConditionParameters_,
                               [this](const Guid& findNodeGuid) { return FindNode(findNodeGuid); },
                               [this](const std::shared_ptr<IAnimationNode>& addNode) { AddCurrentNode(addNode); },
                               [this](const std::shared_ptr<IAnimationNode>& removeNode)
                               {
                                   RemoveCurrentNode(removeNode, -1);
                               },
                               [this](AnimationNodePath* nodePath)
                               {
                                   AddCurrentNodePath(nodePath, -1);
                               });
    }
}

void AnimationTree::AnimationTree::InitForAnimator(const int modelHandle)
{
    currentNodes_.push_back(entryNode_);
    for (const auto& node : nodes_ | std::views::values)
    {
        node->InitForGamePlay(modelHandle);
    }
    
    for (const auto& path : AllNodePaths())
    {
        path->InitNodePath(additionConditionParameters_,
                               [this](const Guid& findNodeGuid) { return FindNode(findNodeGuid); },
                               [this](const std::shared_ptr<IAnimationNode>& addNode) { AddCurrentNode(addNode); },
                               [this, modelHandle](const std::shared_ptr<IAnimationNode>& removeNode)
                               {
                                   RemoveCurrentNode(removeNode, modelHandle);
                               },
                               [this, modelHandle](AnimationNodePath* nodePath)
                               {
                                   AddCurrentNodePath(nodePath, modelHandle);
                               });
    }

    // for (const auto& fromAnyStateNodePath : fromAnyStateNodeNodePaths_)
    // {
    //     fromAnyStateNodePath->SetFromNode(entryNode_);
    // }
}

std::vector<std::shared_ptr<AnimationTree::AnimationNodePath>> AnimationTree::AnimationTree::AllNodePaths() const
{
    std::vector<std::shared_ptr<AnimationNodePath>> nodePaths;
    nodePaths.reserve(fromNodeNodePaths_.size() + fromNodeNodePaths_.size());
    nodePaths.insert (nodePaths.end(), fromNodeNodePaths_        .begin(), fromNodeNodePaths_        .end());
    nodePaths.insert (nodePaths.end(), fromAnyStateNodeNodePaths_.begin(), fromAnyStateNodeNodePaths_.end());
    return nodePaths;
}

void AnimationTree::AnimationTree::OnSave()
{
    std::ofstream ofStream(filePath_);
    if (!ofStream.is_open())
        return;
    
    cereal::JSONOutputArchive archive(ofStream);

    archive(cereal::make_nvp("additionParameters_", additionConditionParameters_));
    archive(cereal::make_nvp("entryNode"          , entryNode_                  ));
    archive(cereal::make_nvp("visualAnyStateNode" , visualAnyStateNode_         ));

    std::size_t count = nodes_.size();
    archive(cereal::make_nvp("nodesCount", count));

    std::size_t index = 0;
    for (const auto& node : nodes_ | std::views::values)
    {
        archive(cereal::make_nvp("nodes_" + std::to_string(index++), node));
    }

    std::size_t pathCount = fromNodeNodePaths_.size();
    archive(cereal::make_nvp("fromNodeNodePathCount", pathCount));

    for (std::size_t i = 0; i < pathCount; ++i)
    {
        archive(cereal::make_nvp("fromNodeNodePath_" + std::to_string(i), fromNodeNodePaths_[i]));
    }

    std::size_t anyPathCount = fromAnyStateNodeNodePaths_.size();
    archive(cereal::make_nvp("fromAnyStateNodeNodePathCount", anyPathCount));

    for (std::size_t i = 0; i < anyPathCount; ++i)
    {
        archive(cereal::make_nvp("fromAnyStateNodeNodePath_" + std::to_string(i), fromAnyStateNodeNodePaths_[i]));
    }
}

void AnimationTree::AnimationTree::OnUpdate(const int modelHandle) const
{
    for (const auto nodesCopy = currentNodes_; const auto& node : nodesCopy)
    {    
        if (!node)
            continue;

        node->OnUpdateAnimation(modelHandle);
    }
    
    if (currentNodePath_)
    {
        currentNodePath_->OnUpdateNodeAnimationBlend();
    }
}

void AnimationTree::AnimationTree::OnDrawGraphEditorGui()
{
    ImGui::Begin(("AnimationTree##" + guid_.Value()).c_str(), nullptr);
    ImDrawList*  drawList   = ImGui::GetWindowDrawList();
    const ImVec2 offset     = ImGui::GetCursorScreenPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();

    //グリッド描画
    static constexpr float K_GRID_STEP = 64.0f;
    static constexpr ImU32 K_GRID_COLOR = IM_COL32(60, 60, 60, 255);
    Gui::Graph::DrawGrid(drawList, offset, windowSize, K_GRID_STEP,K_GRID_COLOR);
    
    //全Nodeの描画
    OnDrawAllNodeGui(drawList, offset);
    OnDrawDraggingNodeGui(drawList, offset);
    
    //確定済み NodePath描画（直線 + 回転矩形）
    for (const auto& path : AllNodePaths())
    {
        const auto fromNodePos   = path->GetVisualFromNodePos();
        const auto targetNodePos = path->GetVisualTargetNodePos();
        const ImVec2 startPosition = offset + ImVec2(fromNodePos.x + 120.0f, fromNodePos.y + 30.0f);
        const ImVec2 endPosition   = offset + ImVec2(targetNodePos.x, targetNodePos.y + 30.0f);
        drawList->AddLine(startPosition, endPosition, IM_COL32(200, 200, 100, 255), 3.0f);

        // --- 回転矩形構築 ---
        const ImVec2 center = (startPosition + endPosition) * 0.5f;
        const ImVec2 diff = endPosition - startPosition;
        const float length = sqrtf (diff.x * diff.x + diff.y * diff.y);
        const float angle  = atan2f(diff.y, diff.x);

        const float halfLength = length * 0.5f;

        ImVec2 corners[4];
        for (int i = 0; i < 4; ++i)
        {
            constexpr float halfThickness = 6.0f;
            const float dx = (i == 0 || i == 3) ? -halfLength : halfLength;
            const float dy = (i < 2) ? -halfThickness : halfThickness;

            const float rx = dx * cosf(angle) - dy * sinf(angle);
            const float ry = dx * sinf(angle) + dy * cosf(angle);
            corners[i] = center + ImVec2(rx, ry);
        }

        // --- 回転矩形描画（視覚補助） ---
        drawList->AddConvexPolyFilled(corners, 4, IM_COL32(255, 255, 255, 30));

        // --- ポリゴン内マウス判定（ImGui未対応なので自前で処理） ---
        auto pointInQuad = [](const ImVec2& p, const ImVec2 quad[4]) -> bool
        {
            auto sign = [](const ImVec2& a, const ImVec2& b, const ImVec2& c)
            {
                return (a.x - c.x) * (b.y - c.y) - (b.x - c.x) * (a.y - c.y);
            };
            const bool b1 = sign(p, quad[0], quad[1]) < 0.0f;
            const bool b2 = sign(p, quad[1], quad[2]) < 0.0f;
            const bool b3 = sign(p, quad[2], quad[3]) < 0.0f;
            const bool b4 = sign(p, quad[3], quad[0]) < 0.0f;
            return b1 == b2 && b2 == b3 && b3 == b4;
        };

        if (ImVec2 mousePos = ImGui::GetMousePos(); pointInQuad(mousePos, corners))
        {
            drawList->AddLine(startPosition, endPosition, IM_COL32(255, 255, 150, 255), 4.0f);

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                for (auto* inspector : Core::Application::ApplicationBase::PopupWindows().Catch<Core::PopupWindow::InspectorWindow>())
                {
                    inspector->TryAddDisplayObject(path);
                }
            }
        }
    }

    // --- 右クリックメニューでノード追加 ---
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup("NodeContextMenu");
    }

    if (ImGui::BeginPopup("NodeContextMenu"))
    {
        if (ImGui::MenuItem("Add AnimationClipNode"))
        {
            const ImVec2 mousePos = ImGui::GetMousePos() - offset;
            const auto newNode = std::make_shared<AnimationClipNode>(glm::vec2(mousePos.x, mousePos.y));
            nodes_[newNode->GetGuid()] = newNode;
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void AnimationTree::AnimationTree::OnDrawAllNodeGui(ImDrawList*  drawList, const ImVec2 offset)
{
    static bool isDragging = false;
    
    //EntryNode 描画・接続処理
    //TODO: このnullcheckは必要性が無い可能性あり
    if (entryNode_)
    {
        auto [isOnBeginDragOutput, isInputHoverReleased_] = entryNode_->OnDrawGraphEditorGui(offset, drawList, entryNode_);
        if (isOnBeginDragOutput)
        {
            dragStartNode_ = entryNode_;
            isDragging = true;
        }
        if (isDragging && isInputHoverReleased_)
        {
            if (const auto fromNode = dragStartNode_.lock())
            {
                if (fromNode != entryNode_)
                {
                    const auto path = std::make_shared<AnimationNodePath>();
                    path->SetFromNode(fromNode);
                    path->SetTargetNode(entryNode_);
                    fromNodeNodePaths_.push_back(path);
                }
            }
            dragStartNode_.reset();
            isDragging = false;
        }
    }

    //VisualAnyStateNode 描画 & 接続処理
    //TODO: このnullcheckは必要性が無い可能性あり
    if (visualAnyStateNode_)
    {
        auto [outputActive_, inputHoveredReleased_] = visualAnyStateNode_->OnDrawGraphEditorGui(offset, drawList, visualAnyStateNode_);
        if (outputActive_)
        {
            dragStartNode_ = visualAnyStateNode_;
            isDragging = true;
        }

        if (isDragging && inputHoveredReleased_)
        {
            if (const auto fromNode = dragStartNode_.lock())
            {
                // AnyStateNode → 他ノード
                for (auto& node : nodes_ | std::views::values)
                {
                    if (visualAnyStateNode_ != node)
                    {
                        auto path = std::make_shared<AnimationNodePath>();
                        path->SetFromNodeForGraphEditorGui(fromNode, fromNode);
                        path->SetTargetNode(node);
                        fromAnyStateNodeNodePaths_.push_back(path);
                    }
                }
            }
            dragStartNode_.reset();
            isDragging = false;
        }
    }

    //通常ノード描画・接続処理
    for (auto& node : nodes_ | std::views::values)
    {
        auto [outputActive_, inputHoveredReleased_] = node->OnDrawGraphEditorGui(offset, drawList, node);
        if (outputActive_)
        {
            dragStartNode_ = node;
            isDragging = true;
        }
        if (isDragging && inputHoveredReleased_)
        {
            if (auto fromNode = dragStartNode_.lock())
            {
                if (fromNode != node)
                {
                    auto path = std::make_shared<AnimationNodePath>();

                    if (visualAnyStateNode_ == fromNode)
                    {
                        path->SetFromNodeForGraphEditorGui(fromNode, fromNode);
                        path->SetTargetNode(node);
                        fromAnyStateNodeNodePaths_.push_back(path);
                    }
                    else
                    {
                        path->SetFromNodeForGraphEditorGui(fromNode, fromNode);
                        path->SetTargetNode(node);
                        fromNodeNodePaths_.push_back(path);
                    }
                }
            }
            dragStartNode_.reset();
            isDragging = false;
        }
    }
}

void AnimationTree::AnimationTree::OnDrawDraggingNodeGui(ImDrawList* drawList, const ImVec2 offset) const
{
    const auto fromNode = dragStartNode_.lock();
    if (fromNode == nullptr)
        return;
    
    const ImVec2 startPos = offset + ImVec2(fromNode->Position().x + 120.0f, fromNode->Position().y + 30.0f);
    const ImVec2 endPos = ImGui::GetMousePos();
    drawList->AddLine(startPos, endPos, IM_COL32(255, 255, 100, 255), 3.0f);
}

void AnimationTree::AnimationTree::OnDrawGui()
{
    ImGui::Begin(filePath_.c_str());
    additionConditionParameters_->OnDrawGui();
    ImGui::End();
}

void AnimationTree::AnimationTree::AddCurrentNode(const std::shared_ptr<IAnimationNode>& node)
{
    currentNodes_.push_back(node);
}

void AnimationTree::AnimationTree::AddCurrentNodePath(AnimationNodePath* nodePath, const int modelHandle)
{
    if (currentNodePath_)
    {
        currentNodePath_->RemoveCurrentNodePath();
    }
    else
    {
        std::erase(currentNodes_, entryNode_);
    }
    for (const auto& fromAnyStateNodePath : fromAnyStateNodeNodePaths_)
    {
        if (fromAnyStateNodePath.get() == nodePath)
            continue;

        fromAnyStateNodePath->SetFromNode(nodePath->GetTargetNode());
    }
    currentNodePath_ = nodePath;
    /** @note Animationが付与されていない条谷状態になる可能性があるため、Nodeを更新してAnimationを付与*/
    nodePath->GetFromNode()->OnUpdateAnimation(modelHandle);
}

void AnimationTree::AnimationTree::RemoveCurrentNode(const std::shared_ptr<IAnimationNode>& node, const int modelHandle)
{
    const auto it = std::ranges::find_if(currentNodes_, [&](const std::shared_ptr<IAnimationNode>& n)
    {
        return n == node;
    });

    if (node != entryNode_)
    {
        currentNodes_.at(0)->OnExitNode(modelHandle);
        std::erase(currentNodes_, currentNodes_.at(0));
    }
}

std::weak_ptr<AnimationTree::IAnimationNode> AnimationTree::AnimationTree::FindNode(const Guid& guid)
{
    if (entryNode_ && entryNode_->GetGuid() == guid)
        return entryNode_;

    if (visualAnyStateNode_ && visualAnyStateNode_->GetGuid() == guid)
        return visualAnyStateNode_;

    const auto it = nodes_.find(guid);
    return it != nodes_.end() ? it->second : nullptr;
}

void AnimationTree::AnimationTree::CreateNode()
{
    const auto node = std::make_shared<AnimationClipNode>();
    nodes_[node->GetGuid()] = node;
}
