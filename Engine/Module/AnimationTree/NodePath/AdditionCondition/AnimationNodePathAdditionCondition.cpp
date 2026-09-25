#include "AnimationNodePathAdditionCondition.h"
#include "../../../Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<bool>, NanamiEngine::Module::AnimationTree::IAnimationNodePathAdditionCondition);
NANAMI_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<int>, NanamiEngine::Module::AnimationTree::IAnimationNodePathAdditionCondition);
NANAMI_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<float>, NanamiEngine::Module::AnimationTree::IAnimationNodePathAdditionCondition);
#pragma endregion
