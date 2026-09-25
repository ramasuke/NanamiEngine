#include "AnimationParameter.h"
#include "../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationParameter<bool>, NanamiEngine::Module::AnimationTree::IAnimationParameter);
NANAMI_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationParameter<int>, NanamiEngine::Module::AnimationTree::IAnimationParameter);
NANAMI_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationParameter<float>, NanamiEngine::Module::AnimationTree::IAnimationParameter);
#pragma endregion
