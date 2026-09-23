#include "AnimationNodePathAdditionCondition.h"
#include "../../../Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<bool>);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<int>);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<float>);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationNodePathAdditionCondition,
    NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<bool>
);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationNodePathAdditionCondition,
    NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<int>
);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationNodePathAdditionCondition,
    NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<float>
);
#pragma endregion
