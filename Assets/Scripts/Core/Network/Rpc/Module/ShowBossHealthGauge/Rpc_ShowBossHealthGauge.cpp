#include "../../Custom_RpcType.h"
#include "../../../../Game/Npc/Enemy/Boss/BossEnemyBase.h"

namespace
{
    // NOTE: 権威側の BT で出したボスHPゲージを、他ピアの同じ NetworkObjectId のボスでも出す
    struct ShowBossHealthGaugeRpcRegistration
    {
        ShowBossHealthGaugeRpcRegistration()
        {
            GameCore::Network::ShowBossHealthGaugeRpc::OnTargeted<GameCore::Npc::BossEnemyBase>(
                [](GameCore::Npc::BossEnemyBase& boss)
                {
                    boss.ShowBossHealthGauge();
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static ShowBossHealthGaugeRpcRegistration s_showBossHealthGaugeRpcRegistration;
}
