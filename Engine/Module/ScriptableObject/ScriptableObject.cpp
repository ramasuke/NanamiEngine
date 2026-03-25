#include "ScriptableObject.h"

NanamiEngine::Module::ScriptableObject::ScriptableObject(std::string contentPath)
    : contentPath_(std::move(contentPath))
{
    
}

void NanamiEngine::Module::ScriptableObject::OnEnableAsset()
{
    
}

void NanamiEngine::Module::ScriptableObject::OnSaveCallback()
{
    std::make_unique<Scriptable::NullContextFile>(contentPath_)->OnSave();
}
