#include "ChangeToMainIslandScene.h"

#include "../../../../../../../Game.h"
#include "../../../../../../../Scene/Main/Content/MainIslandScene/MainIsLandScene.h"
#include "../../../../../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../../../../../../Network/Rpc/Custom_RpcType.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ChangeToMainIslandScene::DoTick(
        const TickContext& context)
    {
        // NOTE: 何度も要求するとロード画面が毎フレーム出し直され、読み込み中にシーンの切り替えが重なる
        if (isRequested_)
            return TickStatus::Abort;
        isRequested_ = true;

        // 権威側限定Tickなら、他ピアも同じシーンへ遷移させる(自分の遷移要求より先に送っておく)
        if (context.IsNetworkAuthority())
        {
            GameCore::Network::ChangeMainSceneRpc::Send(
                context.NetworkObjectId(), Core::Network::DeliveryMode::Reliable, Scene::Main::SceneType::MainIsland, true);
        }

        // ボスを倒して戻るので、ロード画面の地図に踏破の印を押す
        Game::Instance().Scenes().RequestChangeScene(
            Scene::Main::SceneType::MainIsland,
            Scene::Main::SceneTransitionOptions{ .isStageCleared = true });
        
        return TickStatus::Abort;
    }

    void Action::ChangeToMainIslandScene::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("sceneFile_", sceneFile_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChangeToMainIslandScene, GameCore::Npc::Enemy::Behaviour::ActionBase);
#pragma endregion
