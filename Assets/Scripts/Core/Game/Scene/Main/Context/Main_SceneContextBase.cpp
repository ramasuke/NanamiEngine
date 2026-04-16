#include "Main_SceneContextBase.h"

#include "../../../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GameCore::Scene
{
    void SceneContextBase::Init()
    {
        playerSpawnPoint_.InitForPrompty();
    }

    glm::vec3 SceneContextBase::PlayerSpawnPoint() const
    {
        return playerSpawnPoint_->TransformRef().GetWorldPos();
    }

    void SceneContextBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("loadSceneFile_", loadSceneFile_);
        ImGuiHelper::OnDrawInputField("playerSpawnPoint_", playerSpawnPoint_);
    }
}
