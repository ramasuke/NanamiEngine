#include "../../Custom_RpcType.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../GamePlay/Spawn/GamePlay_PrefabSpawner.h"

namespace
{
    // 汎用演出RPC: プレハブを指定位置に生成する(lifeTime_secs > 0 なら時限破棄)。パーティクル等の見た目専用
    struct SpawnPrefabRpcRegistration
    {
        SpawnPrefabRpcRegistration()
        {
            GameCore::Network::SpawnPrefabRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, Guid prefabGuid, glm::vec3 position, float lifeTime_secs)
                {
                    const auto prefab = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<NanamiEngine::Module::Asset::PrefabGameObjectFile>(prefabGuid).lock();
                    if (!prefab)
                    {
                        NanamiEngine::Module::LogWarning("SpawnPrefabRpc: PrefabGameObjectFile が見つかりません (guid:" + prefabGuid.Value() + ")");
                        return;
                    }
                    GamePlay::Spawn::SpawnPrefab(*prefab, position, lifeTime_secs);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static SpawnPrefabRpcRegistration s_spawnPrefabRpcRegistration;
}
