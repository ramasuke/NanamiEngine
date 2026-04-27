#include "Data_FriendlyNpcBaseStatus.h"

namespace NanamiEngine::Module::Asset
{
    FriendlyNpcResources::FriendlyNpcResources(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
        
    }
    
    void FriendlyNpcResources::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("chattingIconPrefab_" , chattingIconPrefab_ );
        ImGuiHelper::OnDrawInputField("chattableIconPrefab_", chattableIconPrefab_);
    }
}
