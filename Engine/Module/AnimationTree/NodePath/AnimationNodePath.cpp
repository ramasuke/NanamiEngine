#include "AnimationNodePath.h"

#include "../../../Core/Application/Time/Time.h"

void AnimationTree::AnimationNodePath::InitNodePath(
    const std::shared_ptr<BlackBoard::ParameterGroup>& additionParams,
    const std::function<std::weak_ptr<IAnimationNode>(const Guid&)>& findNode,
    const std::function<void(const std::shared_ptr<IAnimationNode>&)>& onAddCurrentNode,
    const std::function<void(const std::shared_ptr<IAnimationNode>&)>& onRemoveCurrentNode,
    const std::function<void(AnimationNodePath*)>& onAddNextCurrentNodePath)
{
    additionParams_ = additionParams;
    fromNode_       = findNode(fromNodeGuid_);
    nextNode_       = findNode(nextNodeGuid_);
    visualFromNode_ = findNode(visualFromNodeGuid_);

    onAddCurrentNode_         = onAddCurrentNode;
    onRemoveCurrentNode_      = onRemoveCurrentNode;
    onAddNextCurrentNodePath_ = onAddNextCurrentNodePath;

    SubscribeUpdateNodeAnimationCallback();
}

void AnimationTree::AnimationNodePath::SetFromNode(const std::shared_ptr<IAnimationNode>& node)
{
    isFirstBlendingAnimation_ = true;
    isBlending_               = false;
    transitionDuring_secs_    = 0;
    fromNode_     = node;
    fromNodeGuid_ = node->GetGuid();
    SubscribeUpdateNodeAnimationCallback();
}

void AnimationTree::AnimationNodePath::SetFromNodeForGraphEditorGui(const std::shared_ptr<IAnimationNode>& visualNode,const std::shared_ptr<IAnimationNode>& node)
{
    SetFromNode(node);
    visualFromNode_     = visualNode;
    visualFromNodeGuid_ = visualNode->GetGuid();
}

void AnimationTree::AnimationNodePath::SetTargetNode(const std::shared_ptr<IAnimationNode>& node)
{
    nextNode_     = node           ;
    nextNodeGuid_ = node->GetGuid();
}

void AnimationTree::AnimationNodePath::RemoveCurrentNodePath()
{
    std::cout << "EndBlendAnimation" << std::endl;
    isBlending_ = false;
    onRemoveCurrentNode_(fromNode_.lock());
    nextNode_.lock()->OnUpdateBlendRate(1.0f);
    transitionDuring_secs_ = 0.0f;
}

void AnimationTree::AnimationNodePath::TryAddNextCurrentNodePath(
    const IAnimationNode::UpdateCallbackContext context)
{
    if (fromNode_.lock() == nextNode_.lock())
        return;
    
    if (fromNode_.lock()->GetAnimDuration_secs() - transitionDuration_secs_ < context.during_secs_)
    {
        if (isFirstBlendingAnimation_)
        {
            isFirstBlendingAnimation_ = false;

            if (additionConditionGroup_->Check(*additionParams_))
            {
                // std::cerr << "StartBlendAnimation" << std::endl;
                if (!isBlending_)
                {
                    onAddCurrentNode_(nextNode_.lock());
                    onAddNextCurrentNodePath_(this);
                }
                isBlending_ = true;
            }
        }
    }

    if (fromNode_.lock()->GetAnimDuration_secs() < context.during_secs_)
    {
        isFirstBlendingAnimation_ = true;
    }
}


void AnimationTree::AnimationNodePath::OnUpdateNodeAnimationBlend()
{
    if (isBlending_)
    {
        transitionDuring_secs_ += Time::DeltaTime();
        const float blendRate = std::clamp(transitionDuring_secs_ / transitionDuration_secs_, 0.0f, 1.0f);

        fromNode_.lock()->OnUpdateBlendRate(1 - blendRate);
        nextNode_.lock()->OnUpdateBlendRate(blendRate);
    }
    else
    {
        nextNode_.lock()->OnUpdateBlendRate(1.0f);
    }
}

const Guid& AnimationTree::AnimationNodePath::GetGuid() const
{
    return {};
}

void AnimationTree::AnimationNodePath::SubscribeUpdateNodeAnimationCallback()
{
    const auto node = fromNode_.lock();
    if (!node)
        return;

    const auto old = std::move(fromNodeSubscription_);

    fromNodeSubscription_ = rxcpp::composite_subscription();

    node->OnUpdated().subscribe(
        fromNodeSubscription_,
        [this](const IAnimationNode::UpdateCallbackContext context)
        {
            TryAddNextCurrentNodePath(context);
        });
    
    old.unsubscribe();
}