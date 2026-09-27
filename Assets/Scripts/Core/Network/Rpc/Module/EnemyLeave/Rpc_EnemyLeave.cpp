#include "../../Custom_RpcType.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../Game/Npc/Enemy/EnemyBase.h"

namespace
{
    // 権威側で狩り場から去った敵を、他ピアでも同じ NetworkObjectId の個体でローカル破棄する。倒したわけではないので記録帳には付けない
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
