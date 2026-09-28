#pragma once
#include <cstdint>
#include <optional>
#include <string>

#include "cereal/types/optional.hpp"
#include "cereal/types/string.hpp"
#include "vec3.hpp"
#include "gtc/quaternion.hpp"
#include "../LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Module/Guid/Guid.h"
#include "Engine/Module/Network/Rpc/Engine_Network_Rpc.h"
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

        /** プレイヤー固有 */
        CastSpell,

        /** 設置物 */
        ChargePillarCollapse,

        /** 汎用演出RPC */
        SpawnFollowingPrefab,
        ChargePillarTremble,

        /** プレイヤーの攻撃 */
        PlayerAttackDamage,
        DealDamageText,
        SpawnOrientedPrefab,

        /** 敵固有 */
        PlayAttackWarning,
        EnemyLeave,

        /** 骸竜の砂嵐と光の心臓 */
        BossSandstorm,
        StormHeartShaken,

        /** 演出中の操作ロック */
        PlayerControlLock,

        /** 敵固有 */
        EnemyHealth,
        ShowBossHealthGauge,
    };

    using WakeUpPlayerRpc    = Module::Network::RpcDef<ERpcType::WakeUpPlayer>;
    using SyncAvatarStateRpc = Module::Network::RpcDef<ERpcType::SyncAvatarState, uint8_t>;

    using PlaySeRpc             = Module::Network::RpcDef<ERpcType::PlaySe, Guid, glm::vec3>;
    using PlayBgmRpc            = Module::Network::RpcDef<ERpcType::PlayBgm, Guid>;
    using SpawnPrefabRpc        = Module::Network::RpcDef<ERpcType::SpawnPrefab, Guid, glm::vec3, float>;
    using SpawnMovingPrefabRpc  = Module::Network::RpcDef<ERpcType::SpawnMovingPrefab, Guid, glm::vec3, glm::quat, glm::vec3, float, bool, Damage::PhysicsPower>;
    /** 送り先の NetworkGameObject に付いて行くプレハブ */
    using SpawnFollowingPrefabRpc = Module::Network::RpcDef<ERpcType::SpawnFollowingPrefab, Guid>;
    using PurposeCameraRpc      = Module::Network::RpcDef<ERpcType::PurposeCamera, std::string, int>;
    using ScenePurposeCameraRpc = Module::Network::RpcDef<ERpcType::ScenePurposeCamera, Guid, int>;
    using ChatRpc               = Module::Network::RpcDef<ERpcType::Chat, std::string, Guid>;
    /** 行き先と、ステージを踏破して戻るか */
    using ChangeMainSceneRpc    = Module::Network::RpcDef<ERpcType::ChangeMainScene, Scene::Main::SceneType, bool>;
    using ShakeCameraRpc        = Module::Network::RpcDef<ERpcType::ShakeCamera, float, float>;
    using SetStormRpc           = Module::Network::RpcDef<ERpcType::SetStorm, float, float>;
    using LightningRpc          = Module::Network::RpcDef<ERpcType::Lightning, float, float>;

    using AttackAreaFireRpc     = Module::Network::RpcDef<ERpcType::AttackAreaFire, Damage::PhysicsPower>;
    using EnemyDeathRpc         = Module::Network::RpcDef<ERpcType::EnemyDeath>;
    /** 倒されずに狩り場から去った敵。記録帳には付けずに消す */
    using EnemyLeaveRpc         = Module::Network::RpcDef<ERpcType::EnemyLeave>;

    /** 魔法の guid と、撃った画面で決めた MagicCastTarget */
    using CastSpellRpc          = Module::Network::RpcDef<ERpcType::CastSpell, Guid, glm::vec3, glm::quat, glm::vec3, float>;

    // NOTE: シーンに置かれた設置物は NetworkObjectId を持たないので、送り手の NetworkObjectId 宛てに送って位置で特定する
    /** 倒れた柱の位置と倒れる向き */
    using ChargePillarCollapseRpc = Module::Network::RpcDef<ERpcType::ChargePillarCollapse, glm::vec3, glm::vec3>;
    /** 揺らす中心と半径 */
    using ChargePillarTrembleRpc  = Module::Network::RpcDef<ERpcType::ChargePillarTremble, glm::vec3, float>;

    // NOTE: 敵はホストの所有物なので、他のピアの攻撃は被弾した対象の持ち主へダメージを頼む
    /** 攻撃者の NetworkObjectId と威力 */
    using PlayerAttackDamageRpc  = Module::Network::RpcDef<ERpcType::PlayerAttackDamage, Core::Network::NetworkObjectId, Damage::PhysicsPower>;
    /** ダメージ表記のプレハブ guid と位置と値 */
    using DealDamageTextRpc      = Module::Network::RpcDef<ERpcType::DealDamageText, Guid, glm::vec3, int>;
    /** プレハブ guid と位置、向きと拡大率(無ければプレハブのまま) */
    using SpawnOrientedPrefabRpc = Module::Network::RpcDef<ERpcType::SpawnOrientedPrefab, Guid, glm::vec3, std::optional<glm::quat>, std::optional<float>>;
    /** 送り先の敵のボーンに出す攻撃予兆。IEnemyWarningEffectProvider の guid とボーン名、ボーン空間のオフセット */
    using PlayAttackWarningRpc = Module::Network::RpcDef<ERpcType::PlayAttackWarning, Guid, std::string, glm::vec3>;

    // NOTE: どちらも骸竜の NetworkObjectId 宛て。砂嵐はホストから全員へ、心臓はクライアントからホストへ
    /** 骸竜が呼んだ砂嵐を始めるか止めるかと、止めたのが心臓の揺らぎか */
    using BossSandstormRpc    = Module::Network::RpcDef<ERpcType::BossSandstorm, bool, bool>;
    using StormHeartShakenRpc = Module::Network::RpcDef<ERpcType::StormHeartShaken>;

    /** 演出中に各ピアの Owner の操作を止めるか戻すか。敵の NetworkObjectId 宛て */
    using PlayerControlLockRpc = Module::Network::RpcDef<ERpcType::PlayerControlLock, bool>;

    // NOTE: 敵の HP 同期(SyncParam)が届かないので、ホストが減った HP を敵の NetworkObjectId 宛てに全ピアへ送る
    /** ホストでの現在 HP */
    using EnemyHealthRpc = Module::Network::RpcDef<ERpcType::EnemyHealth, int>;

    /** ボスHPゲージの表示開始。ボスの NetworkObjectId 宛て */
    using ShowBossHealthGaugeRpc = Module::Network::RpcDef<ERpcType::ShowBossHealthGauge>;
}
