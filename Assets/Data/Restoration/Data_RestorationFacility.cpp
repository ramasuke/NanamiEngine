#include "Data_RestorationFacility.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    RestorationFacility::RestorationFacility(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    std::optional<GameCore::Story::StoryFlag> RestorationFacility::RequiredStoryFlag() const
    {
        if (requiredStoryFlag_ < 0)
            return std::nullopt;
        return static_cast<GameCore::Story::StoryFlag>(requiredStoryFlag_);
    }

    std::optional<GameCore::Story::Facility> RestorationFacility::RequiredFacility() const
    {
        if (requiredFacility_ < 0)
            return std::nullopt;
        return static_cast<GameCore::Story::Facility>(requiredFacility_);
    }

    void RestorationFacility::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("facility_", facility_);
        ImGui::TextDisabled("= %s", GameCore::Story::ToString(Facility()).data());
        LibCore::ImGuiHelper::OnDrawInputField("name_", name_);
        LibCore::ImGuiHelper::OnDrawInputField("descriptionLines_", descriptionLines_, [this]
        {
            if (ImGui::Button("Add"))
            {
                descriptionLines_.emplace_back();
            }
        });
        LibCore::ImGuiHelper::OnDrawInputField("cost_", cost_);
        LibCore::ImGuiHelper::OnDrawInputField("requiredStoryFlag_", requiredStoryFlag_);
        if (const auto flag = RequiredStoryFlag())
            ImGui::TextDisabled("= %s", GameCore::Story::ToString(*flag).data());
        LibCore::ImGuiHelper::OnDrawInputField("requiredFacility_", requiredFacility_);
        if (const auto facility = RequiredFacility())
            ImGui::TextDisabled("= %s", GameCore::Story::ToString(*facility).data());
        LibCore::ImGuiHelper::OnDrawInputField("conditionText_", conditionText_);
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(RestorationFacility, RESTORATION_FACILITY_EXTENSION_LABEL, "EventBoard")
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::RestorationFacility);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::RestorationFacility);
#pragma endregion
