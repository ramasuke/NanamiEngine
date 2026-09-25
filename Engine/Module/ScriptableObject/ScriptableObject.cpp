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
NANAMI_REGISTER_TYPE(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::AssetBase);
#pragma endregion
