#include "Sub_SceneContextBase.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Scene::Sub
{
    void SceneContextBase::Initialize()
    {
        DoInitialize();
    }

    void SceneContextBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("sceneFile_", sceneFile_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GameCore::Scene::Sub::SceneContextBase);
#pragma endregion
