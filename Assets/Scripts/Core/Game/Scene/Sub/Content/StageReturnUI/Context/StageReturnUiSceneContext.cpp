#include "StageReturnUiSceneContext.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Scene::Sub
{
    void StageReturnUiSceneContext::DoInitialize()
    {
    }

    void StageReturnUiSceneContext::OnDrawGui()
    {
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Scene::Sub::StageReturnUiSceneContext, GameCore::Scene::Sub::SceneContextBase);
#pragma endregion
