#include "Data_ShopData.h"

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
