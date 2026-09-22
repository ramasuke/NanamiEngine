#include "../../Custom_RpcType.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../GamePlay/Sound/SoundPlayer.h"

namespace
{
    struct PlaySeRpcRegistration
    {
        PlaySeRpcRegistration()
        {
            GameCore::Network::PlaySeRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, Guid soundGuid, glm::vec3 position)
                {
                    const auto sound = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<NanamiEngine::Module::Asset::SoundFile>(soundGuid).lock();
                    if (!sound)
                    {
                        NanamiEngine::Module::LogWarning("PlaySeRpc: SoundFile が見つかりません (guid:" + soundGuid.Value() + ")");
                        return;
                    }
                    GamePlay::Sound::SoundPlayer::PlaySe(*sound, position);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static PlaySeRpcRegistration s_playSeRpcRegistration;
}
