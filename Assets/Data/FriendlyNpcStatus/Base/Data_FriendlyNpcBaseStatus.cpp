#include "Data_FriendlyNpcBaseStatus.h"

namespace NanamiEngine::Module::Asset
{
    FriendlyNpcBaseResources::FriendlyNpcBaseResources(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
        
    }
    
    void FriendlyNpcBaseResources::OnDrawGui()
    {
        ScriptableObject::OnDrawGui();
    }
}
