#include "ScriptableObject.h"
#include "../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module
{
    ScriptableObject::ScriptableObject(std::string contentPath)
        : contentPath_(std::move(contentPath))
    {
    }

    void ScriptableObject::OnEnableAsset()
    {
    }

    void ScriptableObject::OnSaveCallback()
    {
        std::make_unique<Scriptable::NullContextFile>(contentPath_)->OnSave();
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(NanamiEngine::Module::ScriptableObject);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::ScriptableObject);
#pragma endregion
