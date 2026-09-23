#include "AssetBase.h"
#include "../Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::AssetBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Object::IObject, NanamiEngine::Module::Asset::AssetBase);
#pragma endregion
