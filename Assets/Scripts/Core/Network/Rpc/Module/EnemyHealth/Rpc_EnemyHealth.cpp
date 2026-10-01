#include "../../Custom_RpcType.h"
#include "../../../../Game/Npc/Enemy/EnemyBase.h"

namespace
{
    // 権威側の敵 HP を他ピアの同じ個体へ反映する (非権威側の BT にはダメージが入らないため)
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
