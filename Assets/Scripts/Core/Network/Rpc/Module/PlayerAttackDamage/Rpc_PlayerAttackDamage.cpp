#include "../../Custom_RpcType.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../../Game/Damage/Physics/Game_Damage_Physics.h"
#include "../../../../Game/PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"

namespace
{
    // NOTE: 他のピアのプレイヤーの攻撃を、被弾した対象の持ち主が受ける
    struct PlayerAttackDamageRpcRegistration
    {
        PlayerAttackDamageRpcRegistration()
        {
            GameCore::Network::PlayerAttackDamageRpc::OnTargeted<NanamiEngine::Module::Network::NetworkGameObject>(
                [](NanamiEngine::Module::Network::NetworkGameObject& target, NanamiEngine::Core::Network::NetworkObjectId attackerId, GameCore::Damage::PhysicsPower power)
                {
                    const auto targetObject = target.Entity().lock();
                    // NOTE: 攻撃者が居ないとノックバックの向きが決まらないので捨てる
                    const auto attackerObject = NanamiEngine::Module::Network::NetworkRunnerBase::Instance()
                        .DefaultDispatcher().FindNetworkObject(attackerId).lock();
                    if (!targetObject || !attackerObject)
                        return;

                    for (const auto& weakTakable : targetObject->Components().Catches<GameCore::PlayerAvatar::ITakablePlayerAttack>())
                    {
                        if (const auto takable = weakTakable.lock())
                            takable->OnTakeDamage(std::make_unique<GameCore::Damage::Physics>(*attackerObject, *targetObject, power));
                    }
                },
                NanamiEngine::Module::Network::RpcOwnershipFilter::OnlyIfOwner);
        }
    };
    static PlayerAttackDamageRpcRegistration s_playerAttackDamageRpcRegistration;
}
