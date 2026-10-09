#include <optional>

#include "../../Custom_RpcType.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../GamePlay/Sound/SoundPlayer.h"

namespace
{
    // NOTE: 汎用演出RPC: 再生中の BGM を下げて止め、指定があれば次の BGM を上げながら流す
    struct FadeBgmRpcRegistration
    {
        FadeBgmRpcRegistration()
        {
            GameCore::Network::FadeBgmRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, std::optional<Guid> bgmGuid, float fadeOut_secs, float fadeIn_secs)
                {
                    GamePlay::Sound::SoundPlayer::FadeOutAllBgm(fadeOut_secs);
                    if (!bgmGuid)
                        return;

                    const auto bgm = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<NanamiEngine::Module::Asset::SoundFile>(*bgmGuid);
                    if (bgm.expired())
                    {
                        NanamiEngine::Module::LogWarning("FadeBgmRpc: SoundFile が見つかりません (guid:" + bgmGuid->Value() + ")");
                        return;
                    }
                    GamePlay::Sound::SoundPlayer::PlayBgm(bgm, fadeIn_secs);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static FadeBgmRpcRegistration s_fadeBgmRpcRegistration;
}
