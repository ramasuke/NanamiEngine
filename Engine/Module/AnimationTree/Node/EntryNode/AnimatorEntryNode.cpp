#include "AnimatorEntryNode.h"

void AnimationTree::AnimatorEntryNode::InitForGamePlay(int modelHandle)
{
}

void AnimationTree::AnimatorEntryNode::OnUpdateBlendRate(float blendRate)
{
}

void AnimationTree::AnimatorEntryNode::OnUpdateAnimation(int modelHandle, float timeScale)
{
    onUpdate_.OnNext(UpdateCallbackContext(999, 0, timeScale));
}

void AnimationTree::AnimatorEntryNode::OnExitNode(int modelHandle)
{
}
