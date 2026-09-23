#include "TitleSceneContext.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Scene::TitleSceneContext);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Scene::SceneContextBase, GameCore::Scene::TitleSceneContext);
#pragma endregion
