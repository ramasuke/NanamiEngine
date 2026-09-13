#include "AnimationClipNode.h"

#include <algorithm>

#include "DxLib.h"
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
    animationFile_.Init();
    if (animationFile_)
    {
        dxlibAnimationIndex_ = animationFile_->LoadDxLibHandle();
        duration_secs_ = MV1GetAnimTotalTime(dxlibAnimationIndex_, modelAnimationIndex_);
        printf("アニメーションの再生時間: %.2f 秒\n", duration_secs_);
    }
    else
    {
        printf("Animationが設定されていません");
    }
}

float AnimationTree::AnimationClipNode::ClipEndTime() const
{
    if (clipEndTime_ <= 0.0f || clipEndTime_ > duration_secs_)
        return duration_secs_;
    return clipEndTime_;
}

void AnimationTree::AnimationClipNode::OnUpdateAnimation(const int modelHandle, const float timeScale)
{
    const float clipEndTime = ClipEndTime();

    // ノード進入直後（OnExitNode で 0 に戻っている）は再生区間の開始位置から始める
    if (during_secs_ < clipStartTime_)
        during_secs_ = clipStartTime_;

    during_secs_ += Time::DeltaTime() * speed_ * timeScale;

    if (!isLoop_ && during_secs_ > clipEndTime)
        during_secs_ = clipEndTime;

    if (attachedAnimationIndex_ != -1)
    {
        MV1DetachAnim(modelHandle, attachedAnimationIndex_);
    }
    attachedAnimationIndex_ = MV1AttachAnim(modelHandle, modelAnimationIndex_, dxlibAnimationIndex_, false);

    MV1SetAttachAnimTime(modelHandle, attachedAnimationIndex_, during_secs_);
    MV1SetAttachAnimBlendRate(modelHandle, attachedAnimationIndex_, blendRate_);
    onUpdate_.get_subscriber().on_next(UpdateCallbackContext(during_secs_, Time::DeltaTime() * speed_ * timeScale, timeScale));
    if (isLoop_ && during_secs_ >= clipEndTime)
    {
        during_secs_ = clipStartTime_;
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

AnimationTree::ClipProgress AnimationTree::AnimationClipNode::GetClipProgress() const
{
    const float clipEndTime = ClipEndTime();
    const float clipLength  = clipEndTime - clipStartTime_;

    ClipProgress progress;
    progress.duringSecs    = during_secs_;
    progress.durationSecs   = clipEndTime;
    progress.normalizedTime = (clipLength > 0.0f)
                                  ? std::clamp((during_secs_ - clipStartTime_) / clipLength, 0.0f, 1.0f)
                                  : 0.0f;
    return progress;
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
    ImGui::Text("clipTotalTime: %.2f", duration_secs_);
    LibCore::ImGuiHelper::OnDrawInputField("clipStartTime_", clipStartTime_);
    LibCore::ImGuiHelper::OnDrawInputField("clipEndTime_"  , clipEndTime_  );
    LibCore::ImGuiHelper::OnDrawInputField("isLoop_"       , isLoop_       );
    if (ImGui::TreeNode("modelAnimationIndex_"))
    {
        LibCore::ImGuiHelper::OnDrawInputField("modelAnimationIndex_", modelAnimationIndex_);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}
