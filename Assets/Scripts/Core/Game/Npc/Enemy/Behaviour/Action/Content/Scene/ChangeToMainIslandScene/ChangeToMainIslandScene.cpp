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
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChangeToMainIslandScene)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ChangeToMainIslandScene)
#pragma endregion
