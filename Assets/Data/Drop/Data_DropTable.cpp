#include "Data_DropTable.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    DropTable::DropTable(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void DropTable::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("money_", money_);
        LibCore::ImGuiHelper::OnDrawInputField("moneyPickupCount_", moneyPickupCount_);
        LibCore::ImGuiHelper::OnDrawInputField("moneyPickupPrefab_", moneyPickupPrefab_);
        LibCore::ImGuiHelper::OnDrawInputField("items_", items_, [this]
        {
            if (ImGui::Button("Add Item Drop"))
                items_.emplace_back();
        });
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(DropTable, DROP_TABLE_EXTENSION_LABEL, "Item")
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::DropTable);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::DropTable);
#pragma endregion
