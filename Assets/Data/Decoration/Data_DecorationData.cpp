#include "Data_DecorationData.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    DecorationData::DecorationData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void DecorationData::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("name_", name_);
        LibCore::ImGuiHelper::OnDrawInputField("descriptionLines_", descriptionLines_, [this]
        {
            if (ImGui::Button("Add"))
            {
                descriptionLines_.emplace_back();
            }
        });
        LibCore::ImGuiHelper::OnDrawInputField("iconSprite_", iconSprite_);
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(DecorationData, DECORATION_EXTENSION_LABEL, "EventBoard")
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::DecorationData, NanamiEngine::Module::ScriptableObject);
#pragma endregion
