#include "../../Custom_RpcType.h"
#include "../../../../Game/Npc/Enemy/EnemyBase.h"

namespace
{
    // 権威側で減った敵の HP を、他ピアの同じ NetworkObjectId の個体へ反映する。
    // 他ピアの攻撃はホストへ送られるので、非権威側の BT にはダメージが入らない。
    struct EnemyHealthRpcRegistration
    {
        EnemyHealthRpcRegistration()
        {
            GameCore::Network::EnemyHealthRpc::OnTargeted<GameCore::Npc::EnemyBase>(
                [](GameCore::Npc::EnemyBase& enemy, const int health)
                {
                    enemy.ApplyNetworkHealth(health);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static EnemyHealthRpcRegistration s_enemyHealthRpcRegistration;
}
