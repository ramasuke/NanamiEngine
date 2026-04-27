#include "ChangeToMainIslandScene.h"

#include "../../../../../../../Game.h"
#include "../../../../../../../Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "../../../../../../../Scene/Main/Group/Main_GameSceneGroup.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ChangeToMainIslandScene::DoTick(
        const TickContext& context)
    {
        Game::Instance().Scenes().RequestChangeScene<Scene::Main::MainIslandScene>();
        
        return TickStatus::Abort;
    }

    void Action::ChangeToMainIslandScene::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("sceneFile_", sceneFile_);
    }
}
