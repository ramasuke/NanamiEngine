#include "Data_CharacterData.h"

namespace NanamiEngine::Module::Asset
{
    CharacterData::CharacterData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void CharacterData::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("displayName_", displayName_);
        LibCore::ImGuiHelper::OnDrawInputField("reading_", reading_);
        LibCore::ImGuiHelper::OnDrawInputField("tagline_", tagline_);
        LibCore::ImGuiHelper::OnDrawEnumField(
            "avatarType_",
            avatarType_,
            GameCore::PlayerAvatar::PLAYER_AVATAR_TYPES,
            GameCore::PlayerAvatar::ToString);
        LibCore::ImGuiHelper::OnDrawInputField("isUnlocked_", isUnlocked_);
        LibCore::ImGuiHelper::OnDrawInputField("powerPips_", powerPips_);
        LibCore::ImGuiHelper::OnDrawInputField("toughnessPips_", toughnessPips_);
        LibCore::ImGuiHelper::OnDrawInputField("agilityPips_", agilityPips_);
        LibCore::ImGuiHelper::OnDrawInputField("descriptionLines_", descriptionLines_, [this]
        {
            if (ImGui::Button("Add"))
            {
                descriptionLines_.emplace_back();
            }
        });
        LibCore::ImGuiHelper::OnDrawInputField("displayModelPrefab_", displayModelPrefab_);
    }
}
