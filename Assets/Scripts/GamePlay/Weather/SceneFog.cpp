#include "SceneFog.h"

#include "Engine/Core/Platform/Render/Environment.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Weather
{
    void SceneFog::OnStart()
    {
        Apply();
    }

    void SceneFog::OnUpdate()
    {
        Apply();
    }

    void SceneFog::OnDestroy()
    {
        Platform::Render::Environment::SetFogEnabled(false);
    }

    void SceneFog::Apply() const
    {
        Platform::Render::Environment::SetFogEnabled(true);
        Platform::Render::Environment::SetFogColor(fogColor_);
        Platform::Render::Environment::SetFogStartEnd(fogStart_, fogEnd_);
    }

    void SceneFog::OnDrawGui()
    {
        fogColor_.DrawColorEdit("fogColor_");
        ImGuiHelper::OnDrawInputField("fogStart_", fogStart_);
        ImGuiHelper::OnDrawInputField("fogEnd_", fogEnd_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Weather::SceneFog);
#pragma endregion
