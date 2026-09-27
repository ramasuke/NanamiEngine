#include "../../Custom_RpcType.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../Game/Npc/Enemy/Warning/IEnemyWarningEffectProvider.h"

namespace
{
    // 敵の攻撃予兆: 送り先の敵のボーンに、guid の IEnemyWarningEffectProvider で予兆を出す。見た目と音だけ
    struct PlayAttackWarningRpcRegistration
    {
        PlayAttackWarningRpcRegistration()
        {
            GameCore::Network::PlayAttackWarningRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject& target, Guid providerGuid, std::string boneName, glm::vec3 boneOffset)
                {
                    const auto provider = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<GameCore::Npc::Enemy::IEnemyWarningEffectProvider>(providerGuid).lock();
                    if (!provider)
                    {
                        NanamiEngine::Module::LogWarning("PlayAttackWarningRpc: IEnemyWarningEffectProvider が見つかりません (guid:" + providerGuid.Value() + ")");
                        return;
                    }
                    provider->PlayWarning(target.Entity().lock(), boneName, boneOffset);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static PlayAttackWarningRpcRegistration s_playAttackWarningRpcRegistration;
}
