#include "Main_SceneContextBase.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Scene
{
    void SceneContextBase::Init()
    {
        playerSpawnPoint_   .Init();
    }

    glm::vec3 SceneContextBase::PlayerSpawnPoint() const
    {
        return playerSpawnPoint_->Transform().GetWorldPos();
    }

    void SceneContextBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("loadSceneFile_", loadSceneFile_);
        ImGuiHelper::OnDrawInputField("playerSpawnPoint_", playerSpawnPoint_);
        ImGuiHelper::OnDrawInputField("playerAvatarFactory_", playerAvatarFactory_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GameCore::Scene::SceneContextBase);
#pragma endregion
