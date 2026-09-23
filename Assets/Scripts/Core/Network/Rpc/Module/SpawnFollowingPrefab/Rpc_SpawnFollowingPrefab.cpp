#include "../../Custom_RpcType.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../GamePlay/Spawn/GamePlay_PrefabSpawner.h"

namespace
{
    // 汎用演出RPC: 送り先の NetworkGameObject の位置にプレハブを出し、付いて行かせる。パーティクル等の見た目専用
    struct SpawnFollowingPrefabRpcRegistration
    {
        SpawnFollowingPrefabRpcRegistration()
        {
            GameCore::Network::SpawnFollowingPrefabRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject& target, Guid prefabGuid)
                {
                    const auto prefab = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<NanamiEngine::Module::Asset::PrefabGameObjectFile>(prefabGuid).lock();
                    if (!prefab)
                    {
                        NanamiEngine::Module::LogWarning("SpawnFollowingPrefabRpc: PrefabGameObjectFile が見つかりません (guid:" + prefabGuid.Value() + ")");
                        return;
                    }
                    GamePlay::Spawn::SpawnFollowingPrefab(*prefab, target.Entity().lock());
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static SpawnFollowingPrefabRpcRegistration s_spawnFollowingPrefabRpcRegistration;
}
