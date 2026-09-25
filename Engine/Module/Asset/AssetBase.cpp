#include "AssetBase.h"
#include "../Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Object::IObject);
#pragma endregion
