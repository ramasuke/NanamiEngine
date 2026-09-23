#include "IGameObject.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(NanamiEngine::Module::GameObject::IGameObject);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Object::IObject, NanamiEngine::Module::GameObject::IGameObject);
#pragma endregion
