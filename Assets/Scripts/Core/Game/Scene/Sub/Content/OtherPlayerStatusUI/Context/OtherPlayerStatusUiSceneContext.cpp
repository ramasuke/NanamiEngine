#include "OtherPlayerStatusUiSceneContext.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Scene::Sub
{
    void OtherPlayerStatusUiSceneContext::DoInitialize()
    {
    }

    void OtherPlayerStatusUiSceneContext::OnDrawGui()
    {
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Scene::Sub::OtherPlayerStatusUiSceneContext, GameCore::Scene::Sub::SceneContextBase);
#pragma endregion
