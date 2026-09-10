#include "../../Custom_RpcType.h"
#include "../../../../../../../Engine/Core/Application/ApplicationBase.h"
#include "../../../../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../Engine/Core/Object/Registry/ObjectRegistry.h"
#include "../../../../../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../../../../../../../Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../../Data/NpcChatText/Data_NpcChat.h"
#include "../../../../../GamePlay/Ui/NpcChatting/Ui_NpcChatting.h"
#include "../../../../Game/Game.h"
#include "../../../../Game/Scene/Sub/Content/ChattingUI/ChattingUIScene.h"
#include "../../../../Game/Scene/Sub/Group/Sub_GameSceneGroup.h"
#include "../../../../Game/Scene/Sub/Type/SubSceneType.h"

namespace
{
    // OnDisplayChatAsync は参照で受けるため、コルーチンの寿命中は値で保持しておく
    Coroutine::Task<void> ShowChatAsync(
        const std::string displayName,
        const std::shared_ptr<NanamiEngine::Module::Asset::NpcChat> chat)
    {
        const auto& subScenes = GameCore::Game::Instance().SubScenes();
        const auto chattingUIScene = subScenes.Catch<GameCore::Scene::Sub::ChattingUIScene>(GameCore::Scene::Sub::SceneType::ChattingUI);
        co_await chattingUIScene->Context().Npc().OnDisplayChatAsync(displayName, *chat);
    }

    // 汎用演出RPC: 会話 UI を表示する(各ピアが自分で閉じる)
    struct ChatRpcRegistration
    {
        ChatRpcRegistration()
        {
            GameCore::Network::ChatRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, std::string displayName, Guid chatDataGuid)
                {
                    const auto chat = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<NanamiEngine::Module::Asset::NpcChat>(chatDataGuid).lock();
                    if (!chat)
                    {
                        NanamiEngine::Module::LogWarning("ChatRpc: NpcChat が見つかりません (guid:" + chatDataGuid.Value() + ")");
                        return;
                    }
                    Coroutine::StartCoroutine(ShowChatAsync(displayName, chat));
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static ChatRpcRegistration s_chatRpcRegistration;
}
