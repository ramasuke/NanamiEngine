#include "Data_DropTable.h"

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
