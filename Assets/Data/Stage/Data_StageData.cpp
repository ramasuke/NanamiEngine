#include "Data_StageData.h"

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
    }
}
