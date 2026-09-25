#include "Data_FriendlyNpcBaseStatus.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

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

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(FriendlyNpcResources, FRIENDLY_NPC_BASE_RESOURCES_DATA_EXTENSION_LABEL, "Npc::Friendly")
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::FriendlyNpcResources, NanamiEngine::Module::Asset::AssetBase);
#pragma endregion
