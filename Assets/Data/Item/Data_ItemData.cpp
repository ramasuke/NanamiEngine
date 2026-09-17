#include "Data_ItemData.h"

namespace NanamiEngine::Module::Asset
{
    ItemData::ItemData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void ItemData::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("displayName_", displayName_);
        LibCore::ImGuiHelper::OnDrawInputField("iconSprite_", iconSprite_);
        LibCore::ImGuiHelper::OnDrawEnumField("effect_", effect_, ITEM_EFFECT_TYPES, ToString);
        LibCore::ImGuiHelper::OnDrawInputField("effectAmount_", effectAmount_);
        LibCore::ImGuiHelper::OnDrawInputField("effectDuration_secs_", effectDuration_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("useSound_", useSound_);
        LibCore::ImGuiHelper::OnDrawInputField("maxStack_", maxStack_);
    }
}
