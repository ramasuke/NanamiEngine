#include "Main_SceneContextBase.h"

#include "../../../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GameCore::Scene
{
    void SceneContextBase::Init()
    {
        playerSpawnPoint_   .Init();
        swordmanCameraGroup_.Init();
    }

    glm::vec3 SceneContextBase::PlayerSpawnPoint() const
    {
        return playerSpawnPoint_->Transform().GetWorldPos();
    }

    PlayerAvatar::AllPlayerCameraGroup SceneContextBase::CameraGroup() const
    {
        return PlayerAvatar::AllPlayerCameraGroup(
            swordmanCameraGroup_.get());
    }

    void SceneContextBase::BasedOnDrawgui()
    {
        ImGuiHelper::OnDrawInputField("loadSceneFile_", loadSceneFile_);
        ImGuiHelper::OnDrawInputField("playerSpawnPoint_", playerSpawnPoint_);
        ImGuiHelper::OnDrawInputField("swordmanCameraGroup_", swordmanCameraGroup_);
        ImGuiHelper::OnDrawInputField("playerAvatarFactory_", playerAvatarFactory_);
    }
}
