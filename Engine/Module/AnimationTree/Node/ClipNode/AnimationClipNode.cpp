#include "AnimationClipNode.h"

#include "../../../../Core/Application/Time/Time.h"
#include "../../../../Core/Application/Window/Popup/Inspector/InspectorWindow.h"
#include "../../../Gui/Graph/GraphGui.h"
#include "../../../Gui/Graph/NodeOption/NodeOption.h"

AnimationTree::AnimationClipNode::AnimationClipNode(const glm::vec2 position)
    : position_(position)
{
    
}

void AnimationTree::AnimationClipNode::InitForGamePlay(const int modelHandle)
{
    animationFile_.InitForPrompty();
    if (animationFile_)
    {
        dxlibAnimationIndex_ = animationFile_->DxLibHandle();
        duration_secs_ = MV1GetAnimTotalTime(dxlibAnimationIndex_, 0);
        printf("アニメーションの再生時間: %.2f 秒\n", duration_secs_);
    }
    else
    {
        printf("Animationが設定されていません");
    }
}

void AnimationTree::AnimationClipNode::OnUpdateAnimation(const int modelHandle)
{
    during_secs_ += Time::DeltaTime() * speed_;
    if (attachedAnimationIndex_ != -1)
    {
        MV1DetachAnim(modelHandle, attachedAnimationIndex_);
    }
    attachedAnimationIndex_ = MV1AttachAnim(modelHandle, modelAnimationIndex_, dxlibAnimationIndex_, false);

    MV1SetAttachAnimTime(modelHandle, attachedAnimationIndex_, during_secs_);
    MV1SetAttachAnimBlendRate(modelHandle, attachedAnimationIndex_, blendRate_);
    onUpdate_.get_subscriber().on_next(UpdateCallbackContext(during_secs_, Time::DeltaTime() * speed_));
    if (during_secs_ >= duration_secs_)
    {
        during_secs_ = 0;
    }
}

void AnimationTree::AnimationClipNode::OnExitNode(const int modelHandle)
{
    MV1DetachAnim(modelHandle, attachedAnimationIndex_);
    during_secs_ = 0;
    attachedAnimationIndex_ = -1;
}

void AnimationTree::AnimationClipNode::OnUpdateBlendRate(const float blendRate)
{
    blendRate_ = blendRate;
}

Gui::Graph::NodeDrawResult AnimationTree::AnimationClipNode::OnDrawGraphEditorGui(
    const ImVec2& offset,
    ImDrawList* drawList,
    const std::weak_ptr<IAnimationNode> ownPtr)
{
    const Gui::Graph::NodeOption nodeOption
    {
        NODE_VISUAL_STYLE,
        name_,
        true,
        true,
        NODE_SIZE
    };
    return Gui::Graph::DrawNode(offset, position_, drawList, ownPtr, nodeOption, guid_);
}

rxcpp::observable<AnimationTree::IAnimationNode::UpdateCallbackContext> AnimationTree::AnimationClipNode::OnUpdated()
{
    return onUpdate_.get_observable();
}

void AnimationTree::AnimationClipNode::OnDrawGui()
{
    LibCore::ImGuiHelper::OnDrawInputField("animationFile_", animationFile_);
    LibCore::ImGuiHelper::OnDrawInputField("name_"         , name_         );
    LibCore::ImGuiHelper::OnDrawInputField("guid_"         , guid_         );
    LibCore::ImGuiHelper::OnDrawInputField("speed_"        , speed_        );
    LibCore::ImGuiHelper::OnDrawInputField("blendAnimationOffset_secs_", blendAnimationOffset_secs_);
    if (ImGui::TreeNode("modelAnimationIndex_"))
    {
        LibCore::ImGuiHelper::OnDrawInputField("modelAnimationIndex_", modelAnimationIndex_);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}
