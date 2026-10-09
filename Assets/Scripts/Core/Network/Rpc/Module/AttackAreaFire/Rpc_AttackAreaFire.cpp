#include "../../Custom_RpcType.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../Game/Npc/Enemy/AttackArea/Enemy_AttackArea.h"

namespace
{
    // NOTE: 敵の攻撃発火を宛先 AttackArea で再現する。ダメージは自分が所有するアバターにだけ入る
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
