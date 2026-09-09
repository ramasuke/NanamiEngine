#include "../../Custom_RpcType.h"
#include "../../../../../../../Engine/Core/Application/ApplicationBase.h"
#include "../../../../../../../Engine/Core/Object/Registry/ObjectRegistry.h"
#include "../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../../../../../../../Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../GamePlay/Sound/SoundPlayer.h"

namespace
{
    // 汎用演出RPC: 再生中の BGM を全て止めて指定 BGM を流す
    struct PlayBgmRpcRegistration
    {
        PlayBgmRpcRegistration()
        {
            GameCore::Network::PlayBgmRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, Guid bgmGuid)
                {
                    const auto bgm = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<NanamiEngine::Module::Asset::SoundFile>(bgmGuid);
                    if (bgm.expired())
                    {
                        NanamiEngine::Module::LogWarning("PlayBgmRpc: SoundFile が見つかりません (guid:" + bgmGuid.Value() + ")");
                        return;
                    }
                    GamePlay::Sound::SoundPlayer::StopAllBgm();
                    GamePlay::Sound::SoundPlayer::PlayBgm(bgm);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static PlayBgmRpcRegistration s_playBgmRpcRegistration;
}
