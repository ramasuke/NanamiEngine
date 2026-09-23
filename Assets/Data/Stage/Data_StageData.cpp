#include "Data_StageData.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    StageData::StageData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    void StageData::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("displayName_", displayName_);
        LibCore::ImGuiHelper::OnDrawEnumField("sceneType_", sceneType_, GameCore::Scene::Main::SCENE_TYPES, GameCore::Scene::Main::ToString);
        LibCore::ImGuiHelper::OnDrawInputField("mapMarkerPosition_", mapMarkerPosition_);
        LibCore::ImGuiHelper::OnDrawInputField("isCleared_", isCleared_);
        LibCore::ImGuiHelper::OnDrawInputField("thumbnailSprite_", thumbnailSprite_);
        LibCore::ImGuiHelper::OnDrawInputField("elementSprite_", elementSprite_);
        LibCore::ImGuiHelper::OnDrawInputField("difficulty_", difficulty_);
        LibCore::ImGuiHelper::OnDrawInputField("tagText_", tagText_);
        LibCore::ImGuiHelper::OnDrawInputField("descriptionLines_", descriptionLines_, [this]
        {
            if (ImGui::Button("Add"))
            {
                descriptionLines_.emplace_back();
            }
        });
    }
}

#pragma region SerializationMacro
REGISTER_SCRIPTABLE_OBJECT(StageData, STAGE_DATA_EXTENSION_LABEL, "Stage")
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::StageData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::StageData);
#pragma endregion
