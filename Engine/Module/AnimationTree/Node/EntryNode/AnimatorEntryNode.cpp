#include "AnimatorEntryNode.h"
#include "../../../Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimatorEntryNode, NanamiEngine::Module::AnimationTree::IAnimationNode);
#pragma endregion
