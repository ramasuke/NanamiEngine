#include "Animator.h"

#include "../ModelRenderer/ModelRenderer.h"
#include "../../../Core/Application/ApplicationBase.h"
#include "../../../Core/Application/Window/Popup/RunningAnimationTree/RunningAnimationTreeWindow.h"

void Component::Animator::OnAwake()
{
    InitAnimationTree();
}

void Component::Animator::OnStart()
{
    modelDxLibHandle_ = Entity().lock()->Components().Catch<ModelRenderer>().lock()->modelDxLibHandle_;
}

void Component::Animator::OnLateUpdate()
{
    if (!animationTree_)
        return;

    modelDxLibHandle_ = Entity().lock()->Components().Catch<ModelRenderer>().lock()->modelDxLibHandle_;
    animationTree_->OnUpdate(modelDxLibHandle_, timeScale_);
}

void Component::Animator::InitAnimationTree()
{
    if (animationTreeFile_)
    {
        animationTree_ = animationTreeFile_->OnLoadCopyContent();
    }
    if (animationTree_)
    {
        const auto modelDxLibHandle = Components().Catch<ModelRenderer>().lock()->modelDxLibHandle_;
        animationTree_->InitForAnimator(modelDxLibHandle);
    }
}

std::optional<AnimationTree::ClipProgress> Component::Animator::GetClipProgress(const std::string& clipName) const
{
    if (!animationTree_)
        return std::nullopt;

    return animationTree_->GetClipProgress(clipName);
}

std::optional<AnimationTree::ClipProgress> Component::Animator::GetCurrentClipProgress() const
{
    if (!animationTree_)
        return std::nullopt;

    return animationTree_->GetCurrentClipProgress();
}

void Component::Animator::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("animationTreeFile_", animationTreeFile_);
    ImGuiHelper::OnDrawInputField("timeScale_", timeScale_);

    if (animationTree_ && ImGui::Button("Show Running AnimationTree"))
    {
        for (auto* window : Core::Application::ApplicationBase::PopupWindows().Catch<Core::PopupWindow::RunningAnimationTreeWindow>())
            window->TryAddTarget(animationTree_);
    }

    if (animationTree_ && ImGui::TreeNode("Parameter"))
    {
        animationTree_->OnDrawGui();
        
        ImGui::TreePop();
        ImGui::Spacing();
    }
}
