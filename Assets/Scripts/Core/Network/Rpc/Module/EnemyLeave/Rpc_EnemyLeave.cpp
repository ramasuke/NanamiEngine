#include "../../Custom_RpcType.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../Game/Npc/Enemy/EnemyBase.h"

namespace
{
    // NOTE: 権威側で狩り場から去った敵を他ピアでもローカル破棄する。倒していないので記録帳には付けない
    struct EnemyLeaveRpcRegistration
    {
        EnemyLeaveRpcRegistration()
        {
            GameCore::Network::EnemyLeaveRpc::OnTargeted<GameCore::Npc::EnemyBase>(
                [](GameCore::Npc::EnemyBase& enemy)
                {
                    if (const auto entity = enemy.Entity().lock())
                        entity->OnDestroy();
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static EnemyLeaveRpcRegistration s_enemyLeaveRpcRegistration;
}
