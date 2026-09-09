#pragma once
#include <cstdint>
#include <string>

#include "cereal/types/string.hpp"
#include "vec3.hpp"
#include "gtc/quaternion.hpp"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "../../../../../Engine/Module/Guid/Guid.h"
#include "../../../../../Engine/Module/Network/Rpc/Engine_Network_Rpc.h"
#include "../../Game/Damage/Physics/Game_Damage_PhysicsPower.h"
#include "../../Game/Scene/Main/Type/MainSceneType.h"

namespace GameCore::Network
{
    // NOTE: ゲーム側は1,000,000以降を使う規約
    enum class ERpcType : uint32_t
    {
        WakeUpPlayer = 1'000'000,
        SyncAvatarState,

        /** 汎用演出RPC */
        PlaySe,             // (Guid soundGuid, glm::vec3 position)
        PlayBgm,            // (Guid bgmGuid)
        SpawnPrefab,        // (Guid prefabGuid, glm::vec3 position, float lifeTime_secs) lifeTime<=0 なら時限破棄しない
        SpawnMovingPrefab,  // (Guid prefabGuid, glm::vec3 spawnPos, glm::quat rotation, glm::vec3 targetPos, float moveSpeed, bool destroyOnFinish)
        PurposeCamera,      // (std::string childCameraName, int priority) 宛先の子オブジェクトを名前で引く
        ScenePurposeCamera, // (Guid cameraGuid, int priority)
        Chat,               // (std::string displayName, Guid chatDataGuid)
        ChangeMainScene,    // (Scene::Main::SceneType sceneType)
        ShakeCamera,        // (float intensity, float duration) 各ピア自身のカメラを揺らす(対象コンポーネント自体は使わない)

        /** 敵固有 */
        AttackAreaFire,     // (Damage::PhysicsPower power) 宛先は AttackArea 自身の NetworkObjectId
    };

    using WakeUpPlayerRpc    = Module::Network::RpcDef<ERpcType::WakeUpPlayer>;
    using SyncAvatarStateRpc = Module::Network::RpcDef<ERpcType::SyncAvatarState, uint8_t>;

    using PlaySeRpc             = Module::Network::RpcDef<ERpcType::PlaySe, Guid, glm::vec3>;
    using PlayBgmRpc            = Module::Network::RpcDef<ERpcType::PlayBgm, Guid>;
    using SpawnPrefabRpc        = Module::Network::RpcDef<ERpcType::SpawnPrefab, Guid, glm::vec3, float>;
    using SpawnMovingPrefabRpc  = Module::Network::RpcDef<ERpcType::SpawnMovingPrefab, Guid, glm::vec3, glm::quat, glm::vec3, float, bool>;
    using PurposeCameraRpc      = Module::Network::RpcDef<ERpcType::PurposeCamera, std::string, int>;
    using ScenePurposeCameraRpc = Module::Network::RpcDef<ERpcType::ScenePurposeCamera, Guid, int>;
    using ChatRpc               = Module::Network::RpcDef<ERpcType::Chat, std::string, Guid>;
    using ChangeMainSceneRpc    = Module::Network::RpcDef<ERpcType::ChangeMainScene, Scene::Main::SceneType>;
    using ShakeCameraRpc        = Module::Network::RpcDef<ERpcType::ShakeCamera, float, float>;

    using AttackAreaFireRpc     = Module::Network::RpcDef<ERpcType::AttackAreaFire, Damage::PhysicsPower>;
}
