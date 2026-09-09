#include "ChangeToMainIslandScene.h"

#include "../../../../../../../Game.h"
#include "../../../../../../../Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "../../../../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ChangeToMainIslandScene::DoTick(
        const TickContext& context)
    {
        // 権威側限定Tickなら、他ピアも同じシーンへ遷移させる(自分の遷移要求より先に送っておく)
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::ChangeMainSceneRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, Scene::Main::SceneType::MainIsland);
        }

        Game::Instance().Scenes().RequestChangeScene(Scene::Main::SceneType::MainIsland);
        
        return TickStatus::Abort;
    }

    void Action::ChangeToMainIslandScene::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("sceneFile_", sceneFile_);
    }
}
