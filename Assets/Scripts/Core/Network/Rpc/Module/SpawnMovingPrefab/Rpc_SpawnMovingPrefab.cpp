#include "../../Custom_RpcType.h"
#include "../../../../../../../Engine/Core/Application/ApplicationBase.h"
#include "../../../../../../../Engine/Core/Object/Registry/ObjectRegistry.h"
#include "../../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "../../../../../../../Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../GamePlay/Spawn/GamePlay_PrefabSpawner.h"

namespace
{
    // 汎用演出RPC: プレハブを生成して targetPos まで直線移動させる(投射物の見た目専用。ダメージは付与しない)
    struct SpawnMovingPrefabRpcRegistration
    {
        SpawnMovingPrefabRpcRegistration()
        {
            GameCore::Network::SpawnMovingPrefabRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&,
                   Guid prefabGuid, glm::vec3 spawnPos, glm::quat rotation, glm::vec3 targetPos, float moveSpeed, bool destroyOnFinish)
                {
                    const auto prefab = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<NanamiEngine::Module::Asset::PrefabGameObjectFile>(prefabGuid).lock();
                    if (!prefab)
                    {
                        NanamiEngine::Module::LogWarning("SpawnMovingPrefabRpc: PrefabGameObjectFile が見つかりません (guid:" + prefabGuid.Value() + ")");
                        return;
                    }
                    GamePlay::Spawn::SpawnMovingPrefab(*prefab, spawnPos, rotation, targetPos, moveSpeed, destroyOnFinish);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static SpawnMovingPrefabRpcRegistration s_spawnMovingPrefabRpcRegistration;
}
