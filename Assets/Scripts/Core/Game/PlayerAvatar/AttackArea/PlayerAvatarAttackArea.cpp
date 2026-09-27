#include "PlayerAvatarAttackArea.h"
#include "../../../Network/Rpc/Custom_RpcType.h"
#include "../../../../GamePlay/Network/GamePlay_NetworkObjectIdOf.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::PlayerAvatar
{
    void PlayerAttackArea::DoAttack(
        AttackTarget attackTarget,
        std::unique_ptr<IDamage> context)
    {
        attackTarget.Target().OnTakeDamage(std::move(context));
    }

    void PlayerAttackArea::OnRemoteOwnedTarget(
        AttackTarget& attackTarget,
        GameObject::IGameObject& fromObject,
        const Damage::PhysicsPower damagePower)
    {
        const auto targetId   = GamePlay::Network::NetworkObjectIdOf(attackTarget.GameObject());
        const auto attackerId = GamePlay::Network::NetworkObjectIdOf(fromObject);
        if (targetId == Core::Network::NetworkObjectId::Invalid() || attackerId == Core::Network::NetworkObjectId::Invalid())
            return;

        Network::PlayerAttackDamageRpc::Send(targetId, Core::Network::DeliveryMode::Reliable, attackerId, damagePower);
    }
}

#pragma region SerializationMacro
REGISTER_ATTACK_AREA_TYPE(GameCore::PlayerAvatar::ITakablePlayerAttack)
NANAMI_REGISTER_TYPE(GameCore::PlayerAvatar::PlayerAttackArea, GamePlay::AttackArea<GameCore::PlayerAvatar::ITakablePlayerAttack>);
#pragma endregion
