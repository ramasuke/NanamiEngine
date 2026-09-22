#include "../../Custom_RpcType.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../Game/Npc/Enemy/AttackArea/Enemy_AttackArea.h"

namespace
{
    // 敵の攻撃発火を、宛先 AttackArea(子オブジェクト上の NetworkComponent)自身で再現する。
    // 被弾側判定(AttackArea::PhysicsAttack 内の所有者フィルタ)により、自分が所有するアバターにだけダメージが入る。
    struct AttackAreaFireRpcRegistration
    {
        AttackAreaFireRpcRegistration()
        {
            GameCore::Network::AttackAreaFireRpc::OnTargeted<GameCore::Npc::Enemy::AttackArea>(
                [](GameCore::Npc::Enemy::AttackArea& attackArea, GameCore::Damage::PhysicsPower power)
                {
                    const auto fromObject = attackArea.Entity().lock();
                    if (!fromObject)
                        return;
                    attackArea.PhysicsAttack(*fromObject, power);
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::SkipIfOwner);
        }
    };
    static AttackAreaFireRpcRegistration s_attackAreaFireRpcRegistration;
}
