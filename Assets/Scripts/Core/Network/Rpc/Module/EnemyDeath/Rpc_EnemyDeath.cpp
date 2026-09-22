#include "../../Custom_RpcType.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../Game/Npc/Enemy/EnemyBase.h"

namespace
{
    // 権威側で死亡確定した敵を、他ピアでも同じ NetworkObjectId の個体でローカル破棄する。
    // OnDeath(BTリーフ)は権威側限定Tickにより非権威側では呼ばれないため、RPCで明示的に揃える。
    struct EnemyDeathRpcRegistration
    {
        EnemyDeathRpcRegistration()
        {
            GameCore::Network::EnemyDeathRpc::OnTargeted<GameCore::Npc::EnemyBase>(
                [](GameCore::Npc::EnemyBase& enemy)
                {
                    enemy.NotifyDefeated();
                    if (const auto entity = enemy.Entity().lock())
                        entity->OnDestroy();
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static EnemyDeathRpcRegistration s_enemyDeathRpcRegistration;
}
