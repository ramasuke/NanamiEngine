#include "ScriptableObject.h"

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
