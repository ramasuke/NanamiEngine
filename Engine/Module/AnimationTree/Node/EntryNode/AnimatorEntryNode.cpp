#include "AnimatorEntryNode.h"

#include "../../../Gui/Graph/GraphGui.h"

void AnimationTree::AnimatorEntryNode::InitForGamePlay(int modelHandle)
{
}

void AnimationTree::AnimatorEntryNode::OnUpdateBlendRate(float blendRate)
{
}

void AnimationTree::AnimatorEntryNode::OnUpdateAnimation(int modelHandle)
{
    onUpdate_.get_subscriber().on_next(UpdateCallbackContext(999, 0));
}

Gui::Graph::NodeDrawResult AnimationTree::AnimatorEntryNode::OnDrawGraphEditorGui(
    const ImVec2& offset,
    ImDrawList* drawList,
    const std::weak_ptr<IAnimationNode> ownPtr)
{
    const Gui::Graph::NodeOption nodeOption
    {
        NODE_VISUAL_STYLE,
        NODE_NAME,
        false,
        true,
        NODE_SIZE
    };
    return Gui::Graph::DrawNode(offset, position_, drawList, ownPtr, nodeOption, guid_);
}

void AnimationTree::AnimatorEntryNode::OnExitNode(int modelHandle)
{
}
