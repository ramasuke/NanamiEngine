#include "OtherPlayerStatusUiSceneContext.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Scene::Sub
{
    void OtherPlayerStatusUiSceneContext::DoInitialize()
    {
        ui_.Init();
    }

    void OtherPlayerStatusUiSceneContext::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("ui_", ui_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Scene::Sub::OtherPlayerStatusUiSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::Sub::SceneContextBase, GameCore::Scene::Sub::OtherPlayerStatusUiSceneContext);
#pragma endregion
