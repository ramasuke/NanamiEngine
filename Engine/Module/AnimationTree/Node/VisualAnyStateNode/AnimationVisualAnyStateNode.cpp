#include "AnimationVisualAnyStateNode.h"
#include "../../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::AnimationTree
{
    void AnimationVisualAnyStateNode::InitForGamePlay(int modelHandle)
    {
    }

    void AnimationVisualAnyStateNode::OnUpdateAnimation(int modelHandle, float timeScale)
    {
    }

    void AnimationVisualAnyStateNode::OnExitNode(int modelHandle)
    {
    }

    void AnimationVisualAnyStateNode::OnUpdateBlendRate(float blendRate)
    {
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationVisualAnyStateNode);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::AnimationTree::IAnimationNode, NanamiEngine::Module::AnimationTree::AnimationVisualAnyStateNode);
#pragma endregion
