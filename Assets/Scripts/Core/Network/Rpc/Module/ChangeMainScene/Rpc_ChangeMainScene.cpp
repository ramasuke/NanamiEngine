#include "../../Custom_RpcType.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../Game/Game.h"
#include "../../../../Game/Scene/Main/Group/Main_GameSceneGroup.h"

namespace
{
    // 汎用RPC: メインシーンの切り替えを要求する
    struct ChangeMainSceneRpcRegistration
    {
        ChangeMainSceneRpcRegistration()
        {
            GameCore::Network::ChangeMainSceneRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, GameCore::Scene::Main::SceneType sceneType, const bool isStageCleared)
                {
                    GameCore::Game::Instance().Scenes().RequestChangeScene(
                        sceneType,
                        GameCore::Scene::Main::SceneTransitionOptions{ .isStageCleared = isStageCleared });
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static ChangeMainSceneRpcRegistration s_changeMainSceneRpcRegistration;
}
