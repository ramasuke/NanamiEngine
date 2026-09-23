#include "Data_ShopData.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    ShopData::ShopData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void ShopData::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("title_", title_);
        LibCore::ImGuiHelper::OnDrawInputField("entries_", entries_, [this]
        {
            if (ImGui::Button("Add Entry"))
            {
                entries_.emplace_back();
            }
        });
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(ShopData, SHOP_DATA_EXTENSION_LABEL, "Item")
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::ShopData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::ShopData);
#pragma endregion
