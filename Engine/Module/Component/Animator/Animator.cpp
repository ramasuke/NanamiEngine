#include "Animator.h"

#include "../ModelRenderer/ModelRenderer.h"

void Component::Animator::OnAwake()
{
    InitAnimationTree();
}

void Component::Animator::OnLateUpdate()
{
    if (!animationTree_)
        return;

    const auto modelDxLibHandle = Entity().lock()->Components().Catch<ModelRenderer>().lock()->modelDxLibHandle_;
    animationTree_->OnUpdate(modelDxLibHandle);
    for (const auto& animationSync : animationSyncs_)
    {
        animationSync->UpdateSync(modelDxLibHandle);
    }
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
        for (const auto& animationSync : animationSyncs_)
        {
            animationSync->Init(modelDxLibHandle);
        }
    }
}

void Component::Animator::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("animationTreeFile_", animationTreeFile_);
    ImGuiHelper::OnDrawInputField("animationSyncs_", animationSyncs_, [this]
    {
        if (ImGui::TreeNode("Add Syncs"))
        {
            if (ImGui::Button("Transform")) animationSyncs_.push_back(std::make_unique<AnimationTree::TransformSync>());
        
            ImGui::TreePop();
            ImGui::Spacing();
        } 
    });
}
