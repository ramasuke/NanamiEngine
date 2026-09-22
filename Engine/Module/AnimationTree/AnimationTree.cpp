#include "AnimationTree.h"

#include <fstream>
#include <ranges>

#include "imgui_internal.h"
#include "../../Core/Application/Window/Main/Animator/AnimatorWindow.h"
#include "../../Core/Application/Window/Popup/Group/PopupWindowGroup.h"
#include "../../Core/Application/Window/Popup/Inspector/InspectorWindow.h"
#include "../Gui/Graph/Editor/GraphEditorHost.h"
#include "../Serialization/Engine_Module_Serialization.h"
#include "cereal/archives/json.hpp"
#include "Editor/AnimationTreeGraphDelegate.h"
#include "Node/ClipNode/AnimationClipNode.h"
#include "Node/EntryNode/AnimatorEntryNode.h"

AnimationTree::AnimationTree::AnimationTree(std::string filePath)
    : filePath_(std::move(filePath))
{
    // 未作成のファイルは空のツリーとして扱う（新規作成 → Save のフローで使う）。
    // 破損している場合は DeserializeException が投げられ、ツリーは生成されない
    const bool loaded = NanamiEngine::Module::Serialization::LoadJsonFileIfExists(filePath_, [this](cereal::JSONInputArchive& archive)
    {
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
    });
    if (!loaded)
        return;

    for (const auto& path : AllNodePaths())
    {
        path->InitNodePath(additionConditionParameters_,
                               [this](const Guid& findNodeGuid) { return FindNode(findNodeGuid); },
                               [this](const std::shared_ptr<IAnimationNode>& addNode) { AddCurrentNode(addNode); },
                               [this](const std::shared_ptr<IAnimationNode>& removeNode)
                               {
                                   RemoveCurrentNode(removeNode, -1);
                               },
                               [this](AnimationNodePath* nodePath, float timeScale)
                               {
                                   AddCurrentNodePath(nodePath, -1, timeScale);
                               });
    }
}

void AnimationTree::AnimationTree::InitForAnimator(
    const int modelHandle)
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
                               [this, modelHandle](AnimationNodePath* nodePath, float timeScale)
                               {
                                   AddCurrentNodePath(nodePath, modelHandle, timeScale);
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

void AnimationTree::AnimationTree::OnUpdate(const int modelHandle, const float timeScale) const
{
    for (const auto nodesCopy = currentNodes_; const auto& node : nodesCopy)
    {
        if (!node)
            continue;

        node->OnUpdateAnimation(modelHandle, timeScale);
    }

    if (currentNodePath_)
    {
        currentNodePath_->OnUpdateNodeAnimationBlend(timeScale);
    }
}

void AnimationTree::AnimationTree::OnDrawGraphEditorGui(const bool readOnly)
{
    if (!graphHost_)
        graphHost_ = std::make_shared<Gui::Graph::GraphEditorHost>();
    if (!graphDelegate_)
        graphDelegate_ = std::make_shared<AnimationTreeGraphDelegate>();

    // 見出しはファイル名だけにする（filePath_ は UTF-8 なので std::filesystem を通さず区切り文字で切る）
    const std::size_t separator = filePath_.find_last_of("/\\");
    const std::string fileName  = separator == std::string::npos ? filePath_ : filePath_.substr(separator + 1);
    const std::string title     = std::string(readOnly ? "AnimationTree [Running] " : "AnimationTree ") + fileName + "##" + guid_.Value();

    if (ImGui::Begin(title.c_str(), nullptr))
    {
        graphDelegate_->Draw(*this, *graphHost_, readOnly);
    }
    ImGui::End();
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

void AnimationTree::AnimationTree::AddCurrentNodePath(AnimationNodePath* nodePath, const int modelHandle, const float timeScale)
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
    nodePath->GetFromNode()->OnUpdateAnimation(modelHandle, timeScale);
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

AnimationTree::AnimationStateSnapshot AnimationTree::AnimationTree::GetCurrentState() const
{
    AnimationStateSnapshot snap;
    bool primarySet = false;

    for (const auto& node : currentNodes_)
    {
        auto* clip = dynamic_cast<AnimationClipNode*>(node.get());
        if (!clip)
            continue;

        if (!primarySet)
        {
            snap.primaryGuid       = clip->GetGuid();
            snap.primaryDuringSecs = clip->GetDuringSecs();
            snap.primaryBlendRate  = clip->GetBlendRate();
            primarySet = true;
        }
        else
        {
            snap.isBlending          = true;
            snap.secondaryGuid       = clip->GetGuid();
            snap.secondaryDuringSecs = clip->GetDuringSecs();
            snap.secondaryBlendRate  = clip->GetBlendRate();
            break;
        }
    }
    return snap;
}

void AnimationTree::AnimationTree::ApplyRemoteState(const AnimationStateSnapshot& state, const int modelHandle)
{
    for (const auto& node : currentNodes_)
        node->OnExitNode(modelHandle);
    currentNodes_.clear();
    currentNodePath_ = nullptr;

    const auto primaryNode = FindNode(state.primaryGuid).lock();
    if (primaryNode)
    {
        if (auto* clip = dynamic_cast<AnimationClipNode*>(primaryNode.get()))
        {
            clip->SetDuringSecs(state.primaryDuringSecs);
            clip->OnUpdateBlendRate(state.primaryBlendRate);
        }
        currentNodes_.push_back(primaryNode);
    }

    if (state.isBlending)
    {
        const auto secondaryNode = FindNode(state.secondaryGuid).lock();
        if (secondaryNode)
        {
            if (auto* clip = dynamic_cast<AnimationClipNode*>(secondaryNode.get()))
            {
                clip->SetDuringSecs(state.secondaryDuringSecs);
                clip->OnUpdateBlendRate(state.secondaryBlendRate);
            }
            currentNodes_.push_back(secondaryNode);
        }
    }
}

std::optional<AnimationTree::ClipProgress> AnimationTree::AnimationTree::GetClipProgress(const std::string& clipName) const
{
    for (const auto& node : currentNodes_)
    {
        auto* clip = dynamic_cast<AnimationClipNode*>(node.get());
        if (!clip || clip->Name() != clipName)
            continue;

        return clip->GetClipProgress();
    }
    return std::nullopt;
}

std::optional<AnimationTree::ClipProgress> AnimationTree::AnimationTree::GetCurrentClipProgress() const
{
    for (const auto& node : currentNodes_)
    {
        if (auto* clip = dynamic_cast<AnimationClipNode*>(node.get()))
            return clip->GetClipProgress();
    }
    return std::nullopt;
}
