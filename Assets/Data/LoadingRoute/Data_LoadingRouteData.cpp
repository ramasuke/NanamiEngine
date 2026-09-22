#include "Data_LoadingRouteData.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    LoadingRouteData::LoadingRouteData(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
    }

    bool LoadingRouteData::Matches(
        const GameCore::Scene::Main::SceneType from,
        const bool hasFrom,
        const GameCore::Scene::Main::SceneType to) const
    {
        if (to != toScene_)
            return false;

        return isFromAnywhere_ || (hasFrom && from == fromScene_);
    }

    void LoadingRouteData::OnDrawGui()
    {
        using GameCore::Scene::Main::SCENE_TYPES;
        using GameCore::Scene::Main::ToString;

        LibCore::ImGuiHelper::OnDrawInputField("isFromAnywhere_", isFromAnywhere_);
        LibCore::ImGuiHelper::OnDrawEnumField("fromScene_", fromScene_, SCENE_TYPES, ToString);
        LibCore::ImGuiHelper::OnDrawEnumField("toScene_", toScene_, SCENE_TYPES, ToString);
        LibCore::ImGuiHelper::OnDrawInputField("isHover_", isHover_);
        LibCore::ImGuiHelper::OnDrawInputField("hoverCenter_", hoverCenter_);
        LibCore::ImGuiHelper::OnDrawInputField("hoverRadius_", hoverRadius_);
        LibCore::ImGuiHelper::OnDrawInputField("hoverLapSecs_", hoverLapSecs_);
        LibCore::ImGuiHelper::OnDrawInputField("p0_", p0_);
        LibCore::ImGuiHelper::OnDrawInputField("p1_", p1_);
        LibCore::ImGuiHelper::OnDrawInputField("p2_", p2_);
        LibCore::ImGuiHelper::OnDrawInputField("p3_", p3_);
        LibCore::ImGuiHelper::OnDrawInputField("kickerText_", kickerText_);
        LibCore::ImGuiHelper::OnDrawInputField("titleText_", titleText_);
        LibCore::ImGuiHelper::OnDrawInputField("statusText_", statusText_);
        LibCore::ImGuiHelper::OnDrawInputField("fromCaption_", fromCaption_);
        LibCore::ImGuiHelper::OnDrawInputField("fromCaptionPosition_", fromCaptionPosition_);
        LibCore::ImGuiHelper::OnDrawInputField("toCaption_", toCaption_);
        LibCore::ImGuiHelper::OnDrawInputField("toCaptionPosition_", toCaptionPosition_);
        LibCore::ImGuiHelper::OnDrawInputField("hasDestCircle_", hasDestCircle_);
        LibCore::ImGuiHelper::OnDrawInputField("destCirclePosition_", destCirclePosition_);
        LibCore::ImGuiHelper::OnDrawInputField("destCircleScale_", destCircleScale_);
        LibCore::ImGuiHelper::OnDrawInputField("stampPosition_", stampPosition_);
        LibCore::ImGuiHelper::OnDrawInputField("cloudDirection_", cloudDirection_);
        LibCore::ImGuiHelper::OnDrawInputField("hasNetworkStep_", hasNetworkStep_);
    }
}

REGISTER_SCRIPTABLE_OBJECT(LoadingRouteData, LOADING_ROUTE_DATA_EXTENSION_LABEL, "Ui")
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::LoadingRouteData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::LoadingRouteData);
