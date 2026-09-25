#include "IGameObject.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::GameObject::IGameObject, NanamiEngine::Module::Object::IObject);
#pragma endregion
