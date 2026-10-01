#include "../../Custom_RpcType.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../Game/Npc/Enemy/EnemyBase.h"

namespace
{
    // 権威側で死亡確定した敵を他ピアでもローカル破棄する (OnDeath は非権威側で呼ばれないため)
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
