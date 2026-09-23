#include "AnimationParameter.h"
#include "../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationParameter<bool>);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationParameter<int>);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationParameter<float>);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationParameter,
    NanamiEngine::Module::AnimationTree::AnimationParameter<bool>
);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationParameter,
    NanamiEngine::Module::AnimationTree::AnimationParameter<int>
);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationParameter,
    NanamiEngine::Module::AnimationTree::AnimationParameter<float>
);
#pragma endregion
