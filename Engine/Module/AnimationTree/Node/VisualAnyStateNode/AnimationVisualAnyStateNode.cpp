#include "AnimationVisualAnyStateNode.h"
#include "../../../Gui/Graph/GraphGui.h"

namespace NanamiEngine::Module::AnimationTree
{
    void AnimationVisualAnyStateNode::InitForGamePlay(int modelHandle)
    {
    }

    void AnimationVisualAnyStateNode::OnUpdateAnimation(int modelHandle)
    {
    }

    void AnimationVisualAnyStateNode::OnExitNode(int modelHandle)
    {
    }

    Gui::Graph::NodeDrawResult AnimationVisualAnyStateNode::OnDrawGraphEditorGui(
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

    void AnimationVisualAnyStateNode::OnUpdateBlendRate(float blendRate)
    {
    }
}
