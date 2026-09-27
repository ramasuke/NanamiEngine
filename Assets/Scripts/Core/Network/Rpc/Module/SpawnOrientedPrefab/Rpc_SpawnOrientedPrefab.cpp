#include "../../Custom_RpcType.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../../GamePlay/Spawn/GamePlay_PrefabSpawner.h"

namespace
{
    // 汎用演出RPC: プレハブを指定位置・向き・拡大率で生成する。パーティクル等の見た目専用
    struct SpawnOrientedPrefabRpcRegistration
    {
        SpawnOrientedPrefabRpcRegistration()
        {
            GameCore::Network::SpawnOrientedPrefabRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject&, Guid prefabGuid, glm::vec3 position,
                   std::optional<glm::quat> rotation, std::optional<float> scale)
                {
                    const auto prefab = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                        .Catch<NanamiEngine::Module::Asset::PrefabGameObjectFile>(prefabGuid).lock();
                    if (!prefab)
                    {
                        NanamiEngine::Module::LogWarning("SpawnOrientedPrefabRpc: PrefabGameObjectFile が見つかりません (guid:" + prefabGuid.Value() + ")");
                        return;
                    }
                    GamePlay::Spawn::SpawnOrientedPrefab(*prefab, position, rotation, scale);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static SpawnOrientedPrefabRpcRegistration s_spawnOrientedPrefabRpcRegistration;
}
