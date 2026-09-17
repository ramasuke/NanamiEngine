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
        PlaySe,             
        PlayBgm,            
        SpawnPrefab,        
        SpawnMovingPrefab,  
        PurposeCamera,      
        ScenePurposeCamera, 
        Chat,               
        ChangeMainScene,    
        ShakeCamera,        
        SetStorm,           
        Lightning,          

        /** 敵固有 */
        AttackAreaFire,
        EnemyDeath,    
    };

    using WakeUpPlayerRpc    = Module::Network::RpcDef<ERpcType::WakeUpPlayer>;
    using SyncAvatarStateRpc = Module::Network::RpcDef<ERpcType::SyncAvatarState, uint8_t>;

    using PlaySeRpc             = Module::Network::RpcDef<ERpcType::PlaySe, Guid, glm::vec3>;
    using PlayBgmRpc            = Module::Network::RpcDef<ERpcType::PlayBgm, Guid>;
    using SpawnPrefabRpc        = Module::Network::RpcDef<ERpcType::SpawnPrefab, Guid, glm::vec3, float>;
    using SpawnMovingPrefabRpc  = Module::Network::RpcDef<ERpcType::SpawnMovingPrefab, Guid, glm::vec3, glm::quat, glm::vec3, float, bool, Damage::PhysicsPower>;
    using PurposeCameraRpc      = Module::Network::RpcDef<ERpcType::PurposeCamera, std::string, int>;
    using ScenePurposeCameraRpc = Module::Network::RpcDef<ERpcType::ScenePurposeCamera, Guid, int>;
    using ChatRpc               = Module::Network::RpcDef<ERpcType::Chat, std::string, Guid>;
    using ChangeMainSceneRpc    = Module::Network::RpcDef<ERpcType::ChangeMainScene, Scene::Main::SceneType>;
    using ShakeCameraRpc        = Module::Network::RpcDef<ERpcType::ShakeCamera, float, float>;
    using SetStormRpc           = Module::Network::RpcDef<ERpcType::SetStorm, float, float>;
    using LightningRpc          = Module::Network::RpcDef<ERpcType::Lightning, float, float>;

    using AttackAreaFireRpc     = Module::Network::RpcDef<ERpcType::AttackAreaFire, Damage::PhysicsPower>;
    using EnemyDeathRpc         = Module::Network::RpcDef<ERpcType::EnemyDeath>;
}
